# Unit tests

## Executing tests

```
make tests
```

## Writing tests

This project uses [Unity](https://github.com/ThrowTheSwitch/Unity) for unit testing. All test leave inside `tests/` subdirectory. Test suites use the following pattern:

- Each test suite lives inside subdirectory under `tests/`, for example `tests/button/`. 
- Test suite defin# Unit Tests

## Executing Tests

```bash
make tests
````

## Writing Tests

This project uses [Unity](https://github.com/ThrowTheSwitch/Unity) for unit testing.
All tests are located in the `tests/` subdirectory. Test suites follow this structure:

* Each test suite resides in its own subdirectory under `tests/`, for example: `tests/button/`.
* A test suite is defined by a `manifest.mk` file with the following structure:

```
TEST_NAME         := button 
TEST_SOURCES      := tests/button/test_button.c \
                     tests/test_main.c
# Modules under test (real code)
UNDER_TEST        := src/base_components/button.c src/base_components/millis.c
# Real dependencies (if safe to compile on the host)
REAL_DEPS         := 
# Stubs/fakes replacing heavy dependencies
STUBS             := tests/stubs/gpio.c tests/stubs/clock.c

INCLUDES          := -Iinclude -Isrc -Itests/unity/src
CFLAGS_EXTRA      := -DUNIT_TEST
LDFLAGS_EXTRA     :=
```

* A test suite must include a test file with test cases and a `run_all_tests` function, which should call `RUN_TEST(...)` for each test:

```c
void run_all_tests() {
    RUN_TEST(test_simple_press_release);
    ...
}
```

* In tests, direct inclusion of the SDK is avoided because it is not readily compilable on the host.
  Instead, use `tests/sdk_header_stubs` to define SDK headers that the main code includes.

* When a stub is required (e.g., for GPIO or Timer), its implementation is added to `tests/stubs`.
  This stub code is linked into the tests, allowing the test code to interact with the stub to simulate and verify specific conditions.

es itself using `manifest.mk`, that has the following structure:

```
TEST_NAME         := button 
TEST_SOURCES      := tests/button/test_button.c \
                     tests/test_main.c
# modules under test (real code)
UNDER_TEST        := src/base_components/button.c src/base_components/millis.c
# real deps (if they’re harmless on host)
REAL_DEPS         := 
# stubs/fakes replacing heavy deps
STUBS             := tests/stubs/gpio.c tests/stubs/clock.c

INCLUDES          := -Iinclude -Isrc -Itests/unity/src
CFLAGS_EXTRA      := -DUNIT_TEST
LDFLAGS_EXTRA     :=
```

- Test suite defines a test file with some tests and function `run_all_tests` that should `RUN_TEST(...)` all tests:

```
void run_all_tests() {
    RUN_TEST(test_simple_press_release);
    ...
}
```

- In tests, SDK including is avoided, as it is not readely compilable on host. Instead, `tests/sdk_header_stubs` are used to define SDK headers that are being included by main code.

- When some stub is needed, for example for GPIO or Timer, a stub code is added to `tests/stubs`. Then this impl is linked into tests, allowing test code to interact with stub to test conditions it needs to test.





