/* =========================================================================
    Unity - A Test Framework for C
    ThrowTheSwitch.org
    Copyright (c) 2007-26 Mike Karlesky, Mark VanderVoord, & Greg Williams
    SPDX-License-Identifier: MIT
========================================================================= */

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_WhichFails(void)
{
  /* You should see this line fail in your test summary */
  TEST_ASSERT_EQUAL(1, 2);

  /* Notice the rest of these didn't get a chance to run because the line above failed.
   * Unit tests abort each test function on the first sign of trouble.
   * Then NEXT test function runs as normal. */
  TEST_ASSERT_EQUAL(8, 8);
}

void test_WhichPasses(void)
{
  /* You should see this line pass in your test summary */
  TEST_ASSERT_EQUAL(8, 8);
}

void test_anotherWhichPasses(void)
{
  /* You should see this line pass in your test summary */
  TEST_ASSERT_EQUAL(9, 9);
}
