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

void test_sends_first_command_straight_away(void) {

    hal_tasks_init_Ignore();
    

    step_command_handler_t step_command_handler = {};
    setup_step_command_handler(&step_command_handler);

    // Test
    step_command_handler_step_up(&step_command_handler);

    

}