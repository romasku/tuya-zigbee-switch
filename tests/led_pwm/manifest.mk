# Test manifest for LED PWM functionality

TEST_NAME := led_pwm

# Test source files
TEST_SOURCES := tests/led_pwm/test_led_pwm.c \
                tests/test_main.c

# Source files under test
UNDER_TEST := src/base_components/led_pwm.c

# Real dependencies (other source files needed)
REAL_DEPS := src/base_components/led.c

# Stub files (mocked dependencies)
STUBS := tests/stubs/millis.c \
         tests/stubs/gpio.c

# Additional include paths (these are added to the default INCLUDES)
INCLUDES_led_pwm := -Itests/sdk_headers_stubs -DINDICATOR_PWM_SUPPORT -DDEFAULT_INDICATOR_BRIGHTNESS=2 -DUNIT_TEST

# Additional compiler flags
CFLAGS_EXTRA :=

# Additional linker flags
LDFLAGS_EXTRA :=