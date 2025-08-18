# Unit Tests

## Executing Tests

```bash
make tests
```

## Writing Tests

This project uses [Unity](https://github.com/ThrowTheSwitch/Unity) for unit testing.
All tests are located in the `tests/` subdirectory. Test suites follow this pattern:

* Each test suite resides in its own subdirectory under `tests/`, for example: `tests/button/`.
* A test suite is defined by a `manifest.mk` file with the following structure:

```make
TEST_NAME         := button 
TEST_SOURCES      := tests/button/test_button.c \
                     tests/test_main.c
# Modules under test (real code)
UNDER_TEST        := src/base_components/button.c src/base_components/millis.c
# Real dependencies (safe to compile on the host)
REAL_DEPS         := 
# Stubs/fakes replacing heavy dependencies
STUBS             := tests/stubs/gpio.c tests/stubs/clock.c

INCLUDES          := -Iinclude -Isrc -Itests/unity/src
CFLAGS_EXTRA      := -DUNIT_TEST
LDFLAGS_EXTRA     :=
```

* A test suite must include at least one test file with test cases and a `run_all_tests` function, which should call `RUN_TEST(...)` for each test:

```c
void run_all_tests() {
    RUN_TEST(test_simple_press_release);
    ...
}
```

* Direct inclusion of the SDK in tests should be avoided, as it is not readily compilable on the host.
  Instead, use the stub headers in `tests/sdk_header_stubs`, which mirror the SDK headers included by the main code.

* When a stub is required (e.g., for GPIO or Timer), its implementation should be added to `tests/stubs`.
  This stub code is linked into the test binary, allowing the tests to interact with the stub to simulate and verify specific conditions.
