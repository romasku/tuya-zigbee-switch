// tests/support/test_main.c
#include "unity/unity.h"

extern void run_all_tests(void); // provided by each suite via a small file or constructor

int main(void) {
  UNITY_BEGIN();
  run_all_tests();
  return UNITY_END();
}