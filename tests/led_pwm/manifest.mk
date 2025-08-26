# Test manifest for LED PWM functionality

TEST_NAME := led_pwm

# Test source files
TEST_SOURCES := tests/led_pwm/test_led_pwm.c

# Source files under test
UNDER_TEST := src/base_components/led_pwm.c src/device_config/pwm_nv.c

# Real dependencies (other source files needed)
REAL_DEPS := 

# Stub files (mocked dependencies)
STUBS := tests/stubs/millis.c \
         tests/stubs/gpio.c

# Additional includes for this test
INCLUDES_led_pwm := -Itests/sdk_headers_stubs -Itests/stubs

# Additional compiler flags
CFLAGS_EXTRA := -DINDICATOR_PWM_SUPPORT -DDEFAULT_INDICATOR_BRIGHTNESS=2

# Additional linker flags
LDFLAGS_led_pwm :=