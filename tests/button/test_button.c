#include "unity/unity.h"
#include "base_components/button.h"
#include "base_components/millis.h"
#include "stubs/gpio.h"
#include "stubs/clock.h"
#include "chip_8258/gpio.h"
#include <string.h>

static button_t btn;

static void tick_once(void) {
    stub_clock_advance(1);
    millis_update();
    btn_update(&btn);
}

static void simulate_time_ms(u32 ms) {
    for(u32 i=0;i<ms;i++) tick_once();
}

static void press_for(u32 ms){
    gpio_state[btn.pin] = 0;
    tick_once();             
    simulate_time_ms(ms);
}

static void release_for(u32 ms){
    gpio_state[btn.pin] = 1;
    tick_once();
    simulate_time_ms(ms);
}

int press_callback_called = 0;

void press_callback(void *param) {
    press_callback_called += 1;
}

int release_callback_called = 0;

void release_callback(void *param) {
    release_callback_called += 1;
}

int long_press_callback_called = 0;

void long_press_callback(void *param) {
    long_press_callback_called += 1;
}

int multi_press_callback_called = 0;
int multi_press_last_cnt = 0;

void multi_press_callback(void *param, u8 cnt) {
    multi_press_callback_called += 1;
    multi_press_last_cnt = cnt;
}

void setUp(void){
    press_callback_called = 0;
    release_callback_called = 0;
    long_press_callback_called = 0;
    multi_press_callback_called = 0;
    memset(&btn,0,sizeof(btn));
    btn.pin = GPIO_PA0;
    btn.long_press_duration_ms = 100;
    btn.multi_press_duration_ms = 200;
    btn.on_press = press_callback;
    btn.on_release = release_callback;
    btn.on_long_press = long_press_callback;
    btn.on_multi_press = multi_press_callback;

    gpio_state[btn.pin] = 1;
    stub_clock_set(0);
    millis_init();
    btn_init(&btn);
    simulate_time_ms(1000); // Avoid fake multi presses
}

void tearDown(void){}



void test_simple_press_release(void){
    press_for(DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_TRUE(btn.pressed);
    TEST_ASSERT_EQUAL_INT(1, press_callback_called);
    gpio_state[btn.pin]=1;
    release_for(DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_FALSE(btn.pressed);
    TEST_ASSERT_EQUAL_INT(1, release_callback_called);
}

void test_debounce_for_press(void){
  
    for (int i = 0; i < 10; i++) {
        press_for(5);
        release_for(5);
        TEST_ASSERT_FALSE(btn.pressed);
    }
    TEST_ASSERT_FALSE(btn.pressed);
    TEST_ASSERT_EQUAL_INT(0, press_callback_called);
}


void test_debounce_after_pressed(void){
    press_for(DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_TRUE(btn.pressed);

     for (int i = 0; i < 10; i++) {
        press_for(5);
        release_for(5);
        TEST_ASSERT_TRUE(btn.pressed);
    }
    TEST_ASSERT_EQUAL_INT(1, press_callback_called);
    TEST_ASSERT_EQUAL_INT(0, release_callback_called);
}

void test_long_press(void){
    press_for(btn.long_press_duration_ms + DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_TRUE(btn.long_pressed);
    TEST_ASSERT_TRUE(long_press_callback_called == 1);
}

void test_single_press_is_not_multi_press(void){
    press_for(DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_TRUE(multi_press_callback_called == 0);
}

void test_single_press_is_not_multi_after_pause(void){
    press_for(DEBOUNCE_DELAY_MS + 10);
    release_for(btn.multi_press_duration_ms + DEBOUNCE_DELAY_MS + 10);
    press_for(DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_TRUE(multi_press_callback_called == 0);
}

void test_multi_press(void){
    press_for(DEBOUNCE_DELAY_MS + 10);
    release_for(DEBOUNCE_DELAY_MS + 10);
    press_for(DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_EQUAL_INT(2, btn.multi_press_cnt);
    TEST_ASSERT_EQUAL_INT(1, multi_press_callback_called);
    TEST_ASSERT_EQUAL_INT(2, multi_press_last_cnt);
    release_for(DEBOUNCE_DELAY_MS + 10);
    press_for(DEBOUNCE_DELAY_MS + 10);
    TEST_ASSERT_EQUAL_INT(3, btn.multi_press_cnt);
    TEST_ASSERT_EQUAL_INT(2, multi_press_callback_called);
    TEST_ASSERT_EQUAL_INT(3, multi_press_last_cnt);
}

void run_all_tests() {
    RUN_TEST(test_simple_press_release);
    RUN_TEST(test_debounce_for_press);
    RUN_TEST(test_debounce_after_pressed);
    RUN_TEST(test_long_press);
    RUN_TEST(test_single_press_is_not_multi_press);
    RUN_TEST(test_single_press_is_not_multi_after_pause);
    RUN_TEST(test_multi_press);
}