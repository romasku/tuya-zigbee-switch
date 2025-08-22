# PWM Timer Management Test Manifest

TEST_NAME = led_pwm_timer_management
TEST_SOURCES = test_timer_management.c
TEST_INCLUDES = -I../../src
TEST_DEFINES = -DINDICATOR_PWM_SUPPORT -DMAX_PWM_LEDS=4 -DPWM_RESOLUTION_STEPS=16 -DPWM_BASE_FREQUENCY_HZ=500

# Mock dependencies
MOCK_SOURCES = 
MOCK_INCLUDES = 

# Source files to test
SOURCE_FILES = ../../src/base_components/led_pwm.c

# Additional libraries
LIBS = 

# Test-specific flags
CFLAGS = -Wall -Wextra -std=c99