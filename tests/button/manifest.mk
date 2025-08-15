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