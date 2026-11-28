"""Regression test for the Telink millisecond clock.

What the clock has to do
------------------------
``src/telink/hal/timer.c`` builds its millisecond value from the elapsed
tick DELTA, not from the raw tick counter::

    rest_ticks += now - last_tick;
    millis     += rest_ticks / CLOCK_16M_SYS_TIMER_CLK_1MS;
    rest_ticks %= CLOCK_16M_SYS_TIMER_CLK_1MS;

``clock_time()`` is a 32-bit counter at 16 MHz, so it wraps every
2^32 / 16e6 = 268.435456 s.

The implementation this replaces returned ``clock_time() / 16000``
directly. That value spans only 0..268435 ms before it restarts, so it
is a SAWTOOTH, not a timeline: every "now - then" difference is wrong
across the wrap, and a deadline longer than one wrap (the ZCL On With
Timed Off maximum of 6553 s is 24 wraps) cannot be represented at all.

A local 64-bit variant of that idea existed too: it tracked the wrap of
the derived value with a counter shifted left by 32 bits, so every
hardware wrap added 2^32 ms (~49.7 days) instead of the ~268 s that
really elapsed. That variant was observed on real hardware (an AVATTO
relay, ZCL On With Timed Off with onTime=3000 == 300 s):

    command 11:16:58.852 -> OFF 11:18:06.884   (68 s instead of 300 s)
    command 10:42:22.138 -> OFF 10:46:47       (265 s instead of 300 s)

Both OFF timestamps land on a hardware-wrap boundary, and both are far
from the requested deadline.

Why this test compiles C
------------------------
The clock lives in the Telink HAL, which pulls in the vendor SDK
headers, so the host stub cannot exercise it. This test compiles the
shipped ``src/telink/hal/timer.c`` against a fake ``tl_common.h`` and
drives it with a simulated tick, so the logic under test is the logic
that ships.

How the clock is driven
-----------------------
The accumulator only advances when ``hal_millis()`` is called, so the
harness has to model the real calling pattern: ``app_task()`` calls
``hal_millis()`` in every main loop round. The harness therefore polls
the clock at a fixed interval while the simulated tick advances, and
reports the counter at the requested checkpoints.

The harness also carries the two previous implementations as reference
columns, so the tests can assert that they really did fail. If someone
reverts the fix in the Telink HAL, these assertions flip and the suite
goes red.
"""

import os
import subprocess
import textwrap

import pytest

TICKS_PER_MS = 16_000
WRAP_TICKS = 2**32
WRAP_MS = WRAP_TICKS / TICKS_PER_MS  # 268.435456 s

# The largest onTime/offWaitTime the ZCL On With Timed Off command can
# carry: a uint16 in tenths of a second with 0xffff reserved.
ZCL_MAX_TIMER_MS = 6553_400

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TELINK_TIMER = os.path.join(REPO_ROOT, "src", "telink", "hal", "timer.c")

# The fake SDK header the Telink HAL includes. It provides the two
# vendor symbols the clock uses: the raw tick counter and the number of
# ticks per millisecond.
FAKE_TL_COMMON = textwrap.dedent(
    """
    #ifndef FAKE_TL_COMMON_H
    #define FAKE_TL_COMMON_H
    #include <stdint.h>

    #define CLOCK_16M_SYS_TIMER_CLK_1MS 16000

    extern uint32_t g_fake_clock_ticks;

    static inline uint32_t clock_time(void) {
        return g_fake_clock_ticks;
    }
    #endif
    """
)

# C harness. Commands on stdin, one per line:
#
#   poll <interval_ms>   how often the clock is read while time advances
#   at <elapsed_ms>      advance the simulated clock and report
#
# Each "at" prints three values:
#
#   <new> <sawtooth> <shifted64>
#
#   new       the shipped implementation (tick delta accumulator)
#   sawtooth  clock_time() / 16000, the upstream implementation
#   shifted64 the 64-bit variant whose wrap counter was shifted left by
#             32 bits, i.e. 2^32 ms added per hardware wrap
#
# The harness deliberately uses only 32-bit division, like the shipped
# code: the Telink tc32 toolchain has no __udivdi3, so a 64-bit division
# does not link.
HARNESS = textwrap.dedent(
    """
    #include "hal/timer.h"
    #include "tl_common.h"
    #include <stdio.h>
    #include <stdlib.h>

    uint32_t g_fake_clock_ticks = 0;

    /* The upstream implementation: a sawtooth that restarts every
       268.435456 s. */
    static uint32_t sawtooth_millis(void) {
        return clock_time() / 16000;
    }

    /* The 64-bit variant: the wrap of the derived value is tracked, but
       the wrap counter is shifted left by 32 bits, so every hardware
       wrap adds 2^32 ms instead of the ~268 s that really elapsed. */
    static uint32_t shifted_last_lo = 0;
    static uint64_t shifted_base    = 0;

    static uint64_t shifted_millis(void) {
        uint32_t now_lo = clock_time() / 16000;
        if (now_lo < shifted_last_lo) {
            shifted_base += (uint64_t)1 << 32;
        }
        shifted_last_lo = now_lo;
        return shifted_base | (uint64_t)now_lo;
    }

    static uint64_t elapsed_ms   = 0;
    static uint64_t poll_interval = 1;

    static void advance_to(uint64_t target_ms) {
        while (elapsed_ms < target_ms) {
            uint64_t step = target_ms - elapsed_ms;
            if (step > poll_interval) {
                step = poll_interval;
            }
            elapsed_ms += step;
            g_fake_clock_ticks += (uint32_t)(step * 16000ULL);
            /* app_task() reads the clock in every main loop round. */
            hal_millis();
        }
    }

    int main(void) {
        char   cmd[32];
        uint64_t arg = 0;

        while (scanf("%31s %llu", cmd, &arg) == 2) {
            if (cmd[0] == 'p') {
                poll_interval = arg;
            } else if (cmd[0] == 'a') {
                advance_to(arg);
                printf("%u %u %llu\\n",
                       (unsigned)hal_millis(),
                       (unsigned)sawtooth_millis(),
                       (unsigned long long)shifted_millis());
            }
        }
        return 0;
    }
    """
)


@pytest.fixture(scope="module")
def millis_harness(tmp_path_factory):
    """Compile the shipped Telink clock into a runnable helper."""
    tmp = tmp_path_factory.mktemp("telink_millis")
    (tmp / "tl_common.h").write_text(FAKE_TL_COMMON)
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
            str(tmp),
            "-I",
            os.path.join(REPO_ROOT, "src"),
            str(src),
            TELINK_TIMER,
            "-o",
            str(binary),
        ],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        pytest.fail(f"failed to compile the Telink clock harness:\n{result.stderr}")
    return binary


def run_at(harness, checkpoints_ms, poll_interval_ms=100):
    """Advance the simulated clock to each checkpoint and report.

    Returns a list of (new, sawtooth, shifted64) tuples.
    """
    script = [f"poll {poll_interval_ms}"]
    script += [f"at {ms}" for ms in checkpoints_ms]
    result = subprocess.run(
        [str(harness)], input="\n".join(script) + "\n", capture_output=True, text=True
    )
    assert result.returncode == 0, result.stderr

    rows = []
    for line in result.stdout.splitlines():
        if not line.strip():
            continue
        parts = line.split()
        assert len(parts) == 3, line
        rows.append((int(parts[0]), int(parts[1]), int(parts[2])))
    assert len(rows) == len(checkpoints_ms), (
        f"expected {len(checkpoints_ms)} checkpoints, got {len(rows)}"
    )
    return rows


def fire_at(series, checkpoints_ms, delay_ms):
    """The checkpoint at which `series` first reaches its deadline.

    The deadline is armed at the first checkpoint and is `delay_ms`
    later in the counter's own units. Returns the checkpoint index, or
    None.

    The index is the meaningful metric: the previous implementations do
    not count milliseconds across a wrap, so differences taken in their
    units are meaningless.
    """
    target = series[0] + delay_ms
    for index, value in enumerate(series):
        if value >= target:
            return index
    return None


def test_shipped_telink_clock_is_used(millis_harness):
    """The Telink HAL must build the clock from the tick delta."""
    source = open(TELINK_TIMER).read()
    assert "rest_ticks" in source and "last_tick" in source, (
        "the Telink clock no longer accumulates the tick delta"
    )
    # Only 32-bit division: the tc32 toolchain has no __udivdi3, so a
    # 64-bit division in shared code does not link.
    assert "__udivdi3" not in source
    assert "hal/tick64.h" not in source, (
        "the tick64 header is gone; the Telink clock must not use it"
    )


def test_counter_tracks_elapsed_time_exactly(millis_harness):
    """Without a wrap the counter must equal the elapsed time exactly."""
    elapsed = list(range(0, 2_000, 100))
    rows = run_at(millis_harness, elapsed)

    for (new, saw, shifted), at in zip(rows, elapsed):
        assert new == at, f"counter reported {new} ms at {at} ms"
        # Before the first wrap all three agree.
        assert saw == at
        assert shifted == at


def test_survives_single_wrap(millis_harness):
    """Across a hardware wrap the counter must keep following real time."""
    wrap_at_ms = int(WRAP_MS)  # 268435
    elapsed = list(range(wrap_at_ms - 2_000, wrap_at_ms + 2_000, 100))

    rows = run_at(millis_harness, elapsed)

    for (new, saw, _shifted), at in zip(rows, elapsed):
        # The shipped counter is a true millisecond timeline across the wrap.
        assert new == at, f"counter reported {new} ms at {at} ms"

    # The upstream sawtooth restarts at the wrap.
    saw_values = [row[1] for row in rows]
    assert max(saw_values) <= wrap_at_ms, (
        "the upstream sawtooth was expected to stay within one wrap period"
    )
    assert saw_values[-1] < saw_values[0], (
        "the upstream sawtooth was expected to go backwards at the wrap"
    )

    # The 64-bit variant jumps by ~49.7 days at the wrap.
    shifted_values = [row[2] for row in rows]
    jump = max(
        shifted_values[i + 1] - shifted_values[i] for i in range(len(shifted_values) - 1)
    )
    assert jump > 40_000_000, (
        f"expected the 64-bit variant to jump by ~49.7 days at the wrap, got {jump} ms"
    )


def test_wrap_boundary_continuity(millis_harness):
    """The millisecond counter must not skip or repeat across the wrap."""
    wrap_at_ms = int(WRAP_MS)
    elapsed = list(range(wrap_at_ms - 1_000, wrap_at_ms + 1_000, 100))

    rows = run_at(millis_harness, elapsed)
    new_values = [row[0] for row in rows]

    for i in range(1, len(new_values)):
        assert new_values[i] - new_values[i - 1] == 100, (
            f"step {i}: {new_values[i - 1]} -> {new_values[i]}, expected +100"
        )


def test_deadline_longer_than_wrap_is_reached_exactly(millis_harness):
    """A 300 s deadline must not fire early at the wrap boundary.

    This is the actual production failure: onTime=3000 (300 s) is
    longer than the 268.435456 s hardware wrap, so the previous
    implementations fired the relay at the wrap instead of at the
    deadline.
    """
    deadline_ms = 300_000

    # Arm 5 s before a wrap.
    arm_at_ms = int(WRAP_MS) - 5_000
    elapsed = list(range(arm_at_ms, arm_at_ms + deadline_ms + 10_000, 1_000))

    rows = run_at(millis_harness, elapsed)
    new = [row[0] for row in rows]
    saw = [row[1] for row in rows]
    shifted = [row[2] for row in rows]

    # --- shipped implementation: fires at the deadline ---
    index = fire_at(new, elapsed, deadline_ms)
    assert index is not None, "300 s deadline was never reached"
    expected = arm_at_ms + deadline_ms
    assert abs(elapsed[index] - expected) <= 1_000, (
        f"300 s deadline fired at {elapsed[index]} ms, expected ~{expected} ms"
    )

    # --- the 64-bit variant: fired early, at the wrap ---
    old_index = fire_at(shifted, elapsed, deadline_ms)
    assert old_index is not None
    wrap_at = int(WRAP_MS)
    assert elapsed[old_index] < wrap_at + 10_000, (
        f"expected the 64-bit variant to fire at the wrap (~{wrap_at} ms), "
        f"got {elapsed[old_index]} ms"
    )
    assert elapsed[old_index] < expected - 200_000, (
        f"expected the 64-bit variant to fire far earlier than the deadline, "
        f"got {elapsed[old_index]} ms"
    )

    # --- the upstream sawtooth: cannot represent the deadline at all ---
    saw_index = fire_at(saw, elapsed, deadline_ms)
    assert saw_index is None or elapsed[saw_index] > expected + 200_000, (
        f"the upstream sawtooth reached the deadline at {elapsed[saw_index]} ms, "
        f"expected it to miss the deadline entirely"
    )


def test_short_timer_is_unaffected_by_the_fix(millis_harness):
    """A 60 s timer keeps working, before and after the fix."""
    deadline_ms = 60_000

    arm_at_ms = 10_000
    elapsed = list(range(arm_at_ms, arm_at_ms + deadline_ms + 5_000, 1_000))

    rows = run_at(millis_harness, elapsed)

    for label, column in (("new", 0), ("sawtooth", 1), ("shifted64", 2)):
        series = [row[column] for row in rows]
        target = series[0] + deadline_ms
        index = next((i for i, v in enumerate(series) if v >= target), None)
        assert index is not None, f"{label}: 60 s deadline never reached"
        assert elapsed[index] - (arm_at_ms + deadline_ms) <= 1_000, (
            f"{label}: 60 s deadline fired {elapsed[index] - (arm_at_ms + deadline_ms)} ms late"
        )


def test_zcl_maximum_timer_is_reachable(millis_harness):
    """The ZCL On With Timed Off maximum (6553 s) must also work.

    6553 s is the largest value the command can carry: onTime is a
    uint16 in tenths of a second, so it tops out just below an hour
    and a half. That is 24 hardware wraps, which the previous
    implementations could not survive at all.
    """
    arm_at_ms = 1_000
    elapsed = list(range(arm_at_ms, arm_at_ms + ZCL_MAX_TIMER_MS + 10_000, 1_000))

    rows = run_at(millis_harness, elapsed)
    new = [row[0] for row in rows]
    saw = [row[1] for row in rows]
    shifted = [row[2] for row in rows]

    # --- shipped implementation ---
    index = fire_at(new, elapsed, ZCL_MAX_TIMER_MS)
    assert index is not None, "maximum ZCL timer was never reached"
    expected = arm_at_ms + ZCL_MAX_TIMER_MS
    assert abs(elapsed[index] - expected) <= 1_000, (
        f"maximum timer fired at {elapsed[index]} ms, expected ~{expected} ms"
    )

    # --- the 64-bit variant: gave up at the first wrap ---
    old_index = fire_at(shifted, elapsed, ZCL_MAX_TIMER_MS)
    assert old_index is not None
    assert elapsed[old_index] < int(WRAP_MS) + 10_000, (
        f"expected the 64-bit variant to give up at the first wrap, "
        f"got {elapsed[old_index]} ms"
    )
    assert elapsed[old_index] < expected - 6_000_000, (
        f"expected the 64-bit variant to fire far earlier than the deadline, "
        f"got {elapsed[old_index]} ms"
    )

    # --- the upstream sawtooth: the deadline is not representable ---
    saw_index = fire_at(saw, elapsed, ZCL_MAX_TIMER_MS)
    assert saw_index is None or elapsed[saw_index] > expected + 200_000, (
        f"the upstream sawtooth reached the deadline at {elapsed[saw_index]} ms, "
        f"expected it to miss the deadline entirely"
    )


def test_counter_wraps_at_2_32_ms(millis_harness):
    """The clock is the low 32 bits of the millisecond timeline.

    It wraps every 2^32 ms (~49.7 days), which is why the countdown
    deadline is compared with a signed difference instead of an absolute
    comparison.
    """
    wrap_ms = 2**32
    elapsed = [wrap_ms - 10_000, wrap_ms, wrap_ms + 10_000]

    rows = run_at(millis_harness, elapsed, poll_interval_ms=1_000)
    new = [row[0] for row in rows]

    assert new == [wrap_ms - 10_000, 0, 10_000], (
        f"expected the clock to wrap at 2^32 ms, got {new}"
    )
