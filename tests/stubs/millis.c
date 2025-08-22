#include "millis.h"

static u32 mock_millis_value = 0;

u32 millis(void) {
    return mock_millis_value;
}

void millis_set(u32 value) {
    mock_millis_value = value;
}

void millis_advance(u32 ms) {
    mock_millis_value += ms;
}

void millis_init(void) {
    mock_millis_value = 0;
}

void millis_update(void) {
    // No-op for testing
}