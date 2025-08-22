# Device Config PWM Integration Test Manifest

TEST_NAME = device_config_pwm_integration
TEST_SOURCES = test_pwm_integration.c
TEST_INCLUDES = -I../../src
TEST_DEFINES = -DINDICATOR_PWM_SUPPORT -DROUTER -DDEFAULT_INDICATOR_BRIGHTNESS=2

# Mock dependencies
MOCK_SOURCES = 
MOCK_INCLUDES = 

# Source files to test
SOURCE_FILES = 

# Additional libraries
LIBS = 

# Test-specific flags
CFLAGS = -Wall -Wextra -std=c99