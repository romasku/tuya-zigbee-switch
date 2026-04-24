#include "unity.h"
#include "zigbee/step_command_handler.h"
#include "Mocktimer.h"
#include "Mocktasks.h"

int callback_call_times = 0;
void * callback_args_0[10];
int callback_args_1[10];
uint16_t callback_args_2[10];

void mock_callback(void * arg, int change, uint16_t trans_time) {

  callback_args_0[callback_call_times] = arg;
  callback_args_1[callback_call_times] = change;
  callback_args_2[callback_call_times] = trans_time;

  callback_call_times++;
}

void setUp(void)
{
  // Reset
  callback_call_times = 0;


  // Put a space between tests for readability
  printf("\r\n");
}

void tearDown(void)
{
}

void test_first_call_to_step_up_triggers_callback(void) {
    step_command_handler_2_t *step_command_handler = new_step_command_handler();

    int arg = 6;
    step_command_handler->register_callback(step_command_handler, mock_callback, &arg);

    step_command_handler->step_up(step_command_handler);
    
    // Callback was called once, with the expected arguments
    TEST_ASSERT_EQUAL(1, callback_call_times);
    TEST_ASSERT_EQUAL(&arg, callback_args_0[0]); // Check the pointer we got back is the same as the pointer we sent 
    TEST_ASSERT_EQUAL(10, callback_args_1[0]); // A single single step up, should be 10
    TEST_ASSERT_EQUAL(1, callback_args_2[0]); 
}

void test_first_call_to_step_down_triggers_callback(void) {
    step_command_handler_2_t *step_command_handler = new_step_command_handler();

    int arg = 6;
    step_command_handler->register_callback(step_command_handler, mock_callback, &arg);

    step_command_handler->step_down(step_command_handler);
    
    // Callback was called once, with the expected arguments
    TEST_ASSERT_EQUAL(1, callback_call_times);
    TEST_ASSERT_EQUAL(&arg, callback_args_0[0]); // Check the pointer we got back is the same as the pointer we sent 
    TEST_ASSERT_EQUAL(-10, callback_args_1[0]); // A single single step down, should be -10
    TEST_ASSERT_EQUAL(1, callback_args_2[0]); 
}