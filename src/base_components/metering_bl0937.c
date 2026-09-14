#include "metering_bl0937.h"
#include <stddef.h>

#include "hal/printf_selector.h"
#include "hal/timer.h"

// Defaults approximating a common BL0937 plug (calibrate per board!)
#define DEFAULT_INTERVAL_MS              2000
#define DEFAULT_COEF_POWER_MHZ_PER_DW    122   // ~1.22 Hz per W
#define DEFAULT_COEF_VOLTAGE_MHZ_PER_DV  62    // ~1.43 kHz at 230 V
#define DEFAULT_COEF_CURRENT_MHZ_PER_MA  30    // ~30 Hz per A
#define DEFAULT_PULSES_PER_WH            1000

static void _metering_update_callback(void *arg);

static uint32_t freq_mhz(uint32_t pulses, uint32_t elapsed_ms) {
    if (elapsed_ms == 0) {
        return 0;
    }
    // 32-bit only math: the TC32 toolchain does not provide 64-bit
    // division helpers (__udivdi3). Split the *1e6 scaling in two
    // *1000 steps, carrying the remainder for precision.
    if (pulses > 4000000U) {
        pulses = 4000000U; // clamp far above any real BL0937 rate
    }
    uint32_t k = pulses * 1000U;
    uint32_t q = k / elapsed_ms;
    uint32_t r = k % elapsed_ms;
    return q * 1000U + (r * 1000U) / elapsed_ms;
}

int8_t metering_bl0937_init(metering_bl0937_t *m) {
    if (m->interval_ms == 0) {
        m->interval_ms = DEFAULT_INTERVAL_MS;
    }
    if (m->coef_power_mhz_per_dw == 0) {
        m->coef_power_mhz_per_dw = DEFAULT_COEF_POWER_MHZ_PER_DW;
    }
    if (m->coef_voltage_mhz_per_dv == 0) {
        m->coef_voltage_mhz_per_dv = DEFAULT_COEF_VOLTAGE_MHZ_PER_DV;
    }
    if (m->coef_current_mhz_per_ma == 0) {
        m->coef_current_mhz_per_ma = DEFAULT_COEF_CURRENT_MHZ_PER_MA;
    }
    if (m->pulses_per_wh == 0) {
        m->pulses_per_wh = DEFAULT_PULSES_PER_WH;
    }

    // BL0937 pulse outputs are push-pull; count falling edges, no pull.
    m->cf_counter = hal_gpio_counter_init(m->cf_pin, HAL_GPIO_COUNTER_FALLING,
                                          HAL_GPIO_PULL_NONE);
    if (m->cf_counter == HAL_GPIO_COUNTER_INVALID) {
        printf("metering: no counter available for CF\r\n");
        return -1;
    }
    m->cf1_counter = hal_gpio_counter_init(m->cf1_pin, HAL_GPIO_COUNTER_FALLING,
                                           HAL_GPIO_PULL_NONE);
    if (m->cf1_counter == HAL_GPIO_COUNTER_INVALID) {
        printf("metering: no counter available for CF1\r\n");
        hal_gpio_counter_deinit(m->cf_counter);
        m->cf_counter = HAL_GPIO_COUNTER_INVALID;
        return -1;
    }

    // Start measuring current first
    hal_gpio_init(m->sel_pin, 0, HAL_GPIO_PULL_NONE);
    hal_gpio_write(m->sel_pin, m->sel_current_level);
    m->sel_state = 1;

    hal_gpio_counter_start(m->cf_counter);
    hal_gpio_counter_start(m->cf1_counter);
    m->last_sample_ms = hal_millis();

    m->update_task.handler = _metering_update_callback;
    m->update_task.arg     = m;
    hal_tasks_init(&m->update_task);
    hal_tasks_schedule(&m->update_task, m->interval_ms);

    printf("metering: BL0937 driver started (interval %d ms)\r\n",
           (int)m->interval_ms);
    return 0;
}

void metering_bl0937_deinit(metering_bl0937_t *m) {
    hal_tasks_unschedule(&m->update_task);
    if (m->cf_counter != HAL_GPIO_COUNTER_INVALID) {
        hal_gpio_counter_deinit(m->cf_counter);
        m->cf_counter = HAL_GPIO_COUNTER_INVALID;
    }
    if (m->cf1_counter != HAL_GPIO_COUNTER_INVALID) {
        hal_gpio_counter_deinit(m->cf1_counter);
        m->cf1_counter = HAL_GPIO_COUNTER_INVALID;
    }
}

static void _metering_update_callback(void *arg) {
    metering_bl0937_t *m = (metering_bl0937_t *)arg;

    uint32_t now     = hal_millis();
    uint32_t elapsed = now - m->last_sample_ms;
    m->last_sample_ms = now;

    uint32_t cf_pulses  = hal_gpio_counter_read_and_reset(m->cf_counter);
    uint32_t cf1_pulses = hal_gpio_counter_read_and_reset(m->cf1_counter);

    // --- Active power + energy from CF ---
    uint32_t p_mhz = freq_mhz(cf_pulses, elapsed);
    uint32_t p_dw  = p_mhz / m->coef_power_mhz_per_dw;
    m->power_dw = (p_dw > INT16_MAX) ? INT16_MAX : (int16_t)p_dw;

    m->energy_pulses   += cf_pulses;
    m->residual_pulses += cf_pulses;
    while (m->residual_pulses >= m->pulses_per_wh) {
        m->residual_pulses -= m->pulses_per_wh;
        m->energy_wh++;
    }

    // --- Voltage or current from CF1, depending on the finished phase ---
    uint32_t s_mhz = freq_mhz(cf1_pulses, elapsed);
    if (m->sel_state) {
        uint32_t i_ma = s_mhz / m->coef_current_mhz_per_ma;
        m->current_ma = (i_ma > UINT16_MAX) ? UINT16_MAX : (uint16_t)i_ma;
    } else {
        uint32_t v_dv = s_mhz / m->coef_voltage_mhz_per_dv;
        m->voltage_dv = (v_dv > UINT16_MAX) ? UINT16_MAX : (uint16_t)v_dv;
    }

    // Toggle SEL for the next phase
    m->sel_state = !m->sel_state;
    hal_gpio_write(m->sel_pin,
                   m->sel_state ? m->sel_current_level : !m->sel_current_level);

    if (m->on_update != NULL) {
        m->on_update(m->callback_param);
    }

    hal_tasks_schedule(&m->update_task, m->interval_ms);
}
