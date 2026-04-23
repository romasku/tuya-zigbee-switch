#include "unity.h"
#include "zigbee/step_command_handler.h"
#include "Mocktimer.h"
#include "Mocktasks.h"

void setUp(void)
{
  // Put a space between tests for readability
  printf("\r\n");
}

void tearDown(void)
{
}

void test_constuctor(void) {

    step_command_handler_2_t *step_command_handler = new_step_command_handler();
    step_command_handler_2_t *step_command_handler_2 = new_step_command_handler();

    step_command_handler->value = 11;
    step_command_handler_2->value = 20;

    step_command_handler->step_up(step_command_handler);
    step_command_handler_2->step_up(step_command_handler_2);

    TEST_ASSERT_NOT_NULL(step_command_handler);
}