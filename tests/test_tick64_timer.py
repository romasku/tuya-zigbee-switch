"""Regression test for the Telink 64-bit tick/millisecond counter.

The bug this covers
------------------
``src/telink/hal/timer.c`` used to build its 64-bit value like this::

    uint32_t now_lo = clock_time() / CLOCK_16M_SYS_TIMER_CLK_1MS;
    if (s_millis_last_lo != 0 && now_lo < s_millis_last_lo) s_millis_overflows++;
    s_millis_last_lo = now_lo;
    return ((uint64_t)s_millis_overflows << 32) | (uint64_t)s_millis_last_lo;

``clock_time()`` is a 32-bit counter at 16 MHz, so it wraps every
2^32 / 16e6 = 268.435456 s. The derived millisecond value does NOT have
a 32-bit range: it only spans 0..268435 ms before it restarts. Shifting
the wrap counter left by 32 bits therefore added 2^32 ms (~49.7 days)
per hardware wrap instead of the ~268 s that actually elapsed.

The result was not a millisecond timeline. A deadline computed as
``hal_millis_64() + delay`` was compared against a counter that jumped
forward by 49.7 days at every wrap, so any timer longer than ~268 s
fired at the wrap boundary instead of at its deadline.

Observed on real hardware (Shower Switch 0x05EC, ZCL On With Timed Off
with onTime=3000 == 300 s):

    command 11:16:58.852 -> OFF 11:18:06.884   (68 s instead of 300 s)
    command 10:42:22.138 -> OFF 10:46:47       (265 s instead of 300 s)

Both OFF timestamps land exactly on a hardware-wrap boundary.

Why this test compiles C
------------------------
The existing ``test_on_with_timed_off_fires_after_hal_millis_wraparound``
exercises the *stub* timer (``src/stub/hal/timer.c``), which returns a
perfect 64-bit value and therefore cannot reproduce the production bug.
This test compiles ``src/hal/tick64.h`` -- the exact header the
firmware includes -- and drives it with a simulated tick, so the logic
under test is the logic that ships.

The harness also carries the OLD formula, so the tests can assert that
the old behaviour really did fire early. If someone reverts the fix in
the shared header, these assertions flip and the suite goes red.
"""

import os
import re
import subprocess
import textwrap

import pytest

TICK_HZ = 16_000_000
TICKS_PER_MS = 16_000
WRAP_TICKS = 2**32
WRAP_MS = WRAP_TICKS / TICKS_PER_MS  # 268.435456 s

# The largest onTime/offWaitTime the ZCL On With Timed Off command can
# carry: a uint16 in tenths of a second with 0xffff reserved.
ZCL_MAX_TIMER_MS = 6553_400

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TICK64_HEADER = os.path.join(REPO_ROOT, "src", "hal", "tick64.h")

# C harness: reads a script of elapsed milliseconds on stdin and prints
#   <new_millis> <old_millis>
# for each sample, so both formulas can be compared in one run.
#
# The harness deliberately uses only 32-bit division, like the fix: the
# Telink tc32 toolchain has no __udivdi3, so a 64-bit division does not
# link. A 64-bit division in the shared header would break the build.
HARNESS = textwrap.dedent(
    """
    #include "hal/tick64.h"
    #include <stdio.h>

    /* The pre-fix implementation, kept here as the behavioural reference
       for the assertions below. */
    static uint32_t old_last_lo   = 0;
    static uint32_t old_overflows = 0;

    static uint64_t old_millis_64(uint32_t tick) {
        uint32_t now_lo = tick / 16000;
        if (old_last_lo != 0 && now_lo < old_last_lo) {
            old_overflows++;
        }
        old_last_lo = now_lo;
        return ((uint64_t)old_overflows << 32) | (uint64_t)old_last_lo;
    }

    int main(void) {
        hal_tick64_state state;
        hal_tick64_init(&state);

        unsigned long long elapsed_ms;
        while (scanf("%llu", &elapsed_ms) == 1) {
            /* The hardware tick is free running at 16 MHz and wraps at
               2^32, so the tick seen at a given elapsed time is the
               elapsed time modulo the wrap period. */
            uint32_t tick = (uint32_t)((elapsed_ms * 16000ULL) & 0xFFFFFFFFULL);

            printf("%llu %llu\\n",
                   (unsigned long long)hal_tick64_millis(&state, tick),
                   (unsigned long long)old_millis_64(tick));
        }
        return 0;
    }
    """
)


@pytest.fixture(scope="module")
def tick64_harness(tmp_path_factory):
    """Compile the production tick64.h into a runnable helper."""
    tmp = tmp_path_factory.mktemp("tick64")
    src = tmp / "harness.c"
    src.write_text(HARNESS)
    binary = tmp / "harness"

    result = subprocess.run(
        [
            os.environ.get("CC", "cc"),
            "-std=c99",
            "-Wall",
            "-Werror",
            "-I",
            os.path.join(REPO_ROOT, "src"),
            str(src),
            "-o",
            str(binary),
        ],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        pytest.fail(f"failed to compile tick64 harness:\n{result.stderr}")
    return binary


def run_elapsed(harness, elapsed_ms_list):
    """Feed elapsed times to the harness.

    Returns (new_millis, old_millis) lists.
    """
    stdin = "\n".join(str(v) for v in elapsed_ms_list) + "\n"
    result = subprocess.run(
        [str(harness)], input=stdin, capture_output=True, text=True
    )
    assert result.returncode == 0, result.stderr

    new, old = [], []
    for line in result.stdout.splitlines():
        if not line.strip():
            continue
        n, o = line.split()
        new.append(int(n))
        old.append(int(o))
    return new, old


def fire_after(series, elapsed_ms_list, delay_ms):
    """Real elapsed time at which `series` first reaches its deadline.

    The deadline is armed at the first sample and is `delay_ms` later in
    the counter's own units. Returns the elapsed time (not the counter
    value) at which it was reached, or None.

    Measuring in elapsed time is the only meaningful metric here: the
    old counter's units are not milliseconds across a wrap -- it jumps
    by 2^32 ms -- so differences taken in its units are meaningless.
    """
    target = series[0] + delay_ms
    for value, elapsed in zip(series, elapsed_ms_list):
        if value >= target:
            return elapsed
    return None


def test_tick64_header_exists():
    """The shared header the firmware includes must exist."""
    assert os.path.exists(TICK64_HEADER), TICK64_HEADER


def test_no_64bit_division_in_shared_header():
    """The header must not use 64-bit division.

    The Telink tc32 toolchain has no __udivdi3, so a 64-bit division
    in shared code does not link -- the build fails with an undefined
    reference. This catches the mistake before a container build does.
    """
    source = open(TICK64_HEADER).read()
    # Strip both comment styles so a mention in prose does not trip
    # the check.
    code = re.sub(r"/\*.*?\*/", "", source, flags=re.S)
    code = "\n".join(line.split("//")[0] for line in code.splitlines())

    assert "__udivdi3" not in code

    # The only division present must divide a 32-bit value.
    assert "delta / HAL_TICK64_TICKS_PER_MS" in code, (
        "expected the 32-bit division of the tick delta"
    )
    for line in code.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("*"):
            continue
        if "/=" not in stripped:
            continue
        divisor = stripped.split("/=")[1].strip().rstrip(";").strip()
        if divisor.startswith("UINT64") or divisor.startswith("u64"):
            pytest.fail(f"64-bit division in shared header: {stripped}")
        if divisor.isdigit() and int(divisor) > 0xFFFFFFFF:
            pytest.fail(f"64-bit divisor literal in shared header: {stripped}")


def test_counter_tracks_elapsed_time_exactly(tick64_harness):
    """Without a wrap the counter must equal the elapsed time exactly."""
    elapsed = list(range(0, 500))
    new, old = run_elapsed(tick64_harness, elapsed)

    assert new == elapsed
    assert old == elapsed  # the old formula agreed before the first wrap


def test_survives_single_wrap(tick64_harness):
    """Across a hardware wrap the counter must keep following real time."""
    # 268.435456 s is where the 32-bit tick wraps.
    wrap_at_ms = int(WRAP_MS)  # 268435
    elapsed = list(range(wrap_at_ms - 500, wrap_at_ms + 500))

    new, old = run_elapsed(tick64_harness, elapsed)

    # The new counter is a true millisecond timeline across the wrap.
    assert new == elapsed, "counter diverged from real time across the wrap"

    # The old one is not: it jumps by ~49.7 days at the wrap.
    jump = max(old[i + 1] - old[i] for i in range(len(old) - 1))
    assert jump > 40_000_000, (
        f"expected the old formula to jump by ~49.7 days at the wrap, got {jump} ms"
    )


def test_wrap_boundary_continuity(tick64_harness):
    """The millisecond counter must not skip or repeat across the wrap."""
    wrap_at_ms = int(WRAP_MS)
    elapsed = list(range(wrap_at_ms - 50, wrap_at_ms + 50))

    new, _old = run_elapsed(tick64_harness, elapsed)

    for i in range(1, len(new)):
        assert new[i] - new[i - 1] == 1, (
            f"step {i}: {new[i - 1]} -> {new[i]}, expected +1"
        )


def test_deadline_longer_than_wrap_is_reached_exactly(tick64_harness):
    """A 300 s deadline must not fire early at the wrap boundary.

    This is the actual production failure: onTime=3000 (300 s) is
    longer than the 268.435456 s hardware wrap, so the old code fired
    the relay at the wrap instead of at the deadline.
    """
    deadline_ms = 300_000

    # Arm 5 s before a wrap: the old code would have fired ~5 s later.
    arm_at_ms = int(WRAP_MS) - 5_000
    elapsed = list(range(arm_at_ms, arm_at_ms + deadline_ms + 10_000, 100))

    new, old = run_elapsed(tick64_harness, elapsed)

    # --- new implementation: fires at the deadline ---
    fired = fire_after(new, elapsed, deadline_ms)
    assert fired is not None, "300 s deadline was never reached"
    expected = arm_at_ms + deadline_ms
    assert abs(fired - expected) <= 100, (
        f"300 s deadline fired at {fired} ms, expected ~{expected} ms"
    )

    # --- old implementation: fired early, at the wrap ---
    old_fired = fire_after(old, elapsed, deadline_ms)
    assert old_fired is not None
    wrap_at = int(WRAP_MS)
    assert old_fired < wrap_at + 10_000, (
        f"expected the old formula to fire at the wrap (~{wrap_at} ms), "
        f"got {old_fired} ms"
    )
    assert old_fired < expected - 200_000, (
        f"expected the old formula to fire far earlier than the deadline, "
        f"got {old_fired} ms"
    )


def test_short_timer_is_unaffected_by_the_fix(tick64_harness):
    """A 60 s timer keeps working, both before and after the fix."""
    deadline_ms = 60_000

    # Arm well inside an epoch so no wrap falls inside the window.
    arm_at_ms = 10_000
    elapsed = list(range(arm_at_ms, arm_at_ms + deadline_ms + 5_000, 100))

    new, old = run_elapsed(tick64_harness, elapsed)

    for label, series in (("new", new), ("old", old)):
        target = series[0] + deadline_ms
        fired = next((m for m in series if m >= target), None)
        assert fired is not None, f"{label}: 60 s deadline never reached"
        assert fired - target <= 100, f"{label}: fired {fired - target} ms late"


def test_zcl_maximum_timer_is_reachable(tick64_harness):
    """The ZCL On With Timed Off maximum (6553.4 s) must also work.

    6553.4 s is the largest value the command can carry: onTime is a
    uint16 in tenths of a second with 0xffff reserved, so 65534 * 0.1 s.
    That is 24.4 hardware wraps, which the old code could not survive
    at all.
    """
    arm_at_ms = 1_000
    elapsed = list(range(arm_at_ms, arm_at_ms + ZCL_MAX_TIMER_MS + 10_000, 1_000))

    new, old = run_elapsed(tick64_harness, elapsed)

    # --- new implementation ---
    fired = fire_after(new, elapsed, ZCL_MAX_TIMER_MS)
    assert fired is not None, "maximum ZCL timer was never reached"
    expected = arm_at_ms + ZCL_MAX_TIMER_MS
    assert abs(fired - expected) <= 1_000, (
        f"maximum timer fired at {fired} ms, expected ~{expected} ms"
    )

    # --- old implementation: gave up at the first wrap ---
    old_fired = fire_after(old, elapsed, ZCL_MAX_TIMER_MS)
    assert old_fired is not None
    assert old_fired < int(WRAP_MS) + 10_000, (
        f"expected the old formula to give up at the first wrap, "
        f"got {old_fired} ms"
    )
    assert old_fired < expected - 6_000_000, (
        f"expected the old formula to fire far earlier than the deadline, "
        f"got {old_fired} ms"
    )
