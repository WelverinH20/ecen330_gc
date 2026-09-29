#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/gptimer.h"
#include "hw.h"
#include "lcd.h"
#include "pin.h"
#include "watch.h"

static const char *TAG = "lab03";

//Step 4 Global Variables
static volatile uint32_t timer_ticks = 0;
static volatile bool running = false;

//Step 4 timer call back:
static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    //button check
    if (pin_get_level(HW_BTN_A) == 0) {
        running = true;
    }
    if (pin_get_level(HW_BTN_B) == 0) {
        running = false;
    }
    if (pin_get_level(HW_BTN_START) == 0) {
        running = false;
        timer_ticks = 0;
    }
    // Increment time if stopwatch is active
    if (running) {
        timer_ticks++;
    }

    return false;
}

// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "Starting");

    // Step 3: Configure Button GPIO Pins as Inputs
    pin_reset(HW_BTN_A);
    pin_input(HW_BTN_A,true);

    pin_reset(HW_BTN_B);
    pin_input(HW_BTN_B,true);

    pin_reset(HW_BTN_START);
    pin_input(HW_BTN_START,true);

    // Step 5: Configure GPTimer
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz resolution (1 tick = 1 us)
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Register callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    // Alarm action: 10 ms alarm (10,000 microseconds @ 1 MHz)
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 10000, // 10 ms interval (1/100th sec)
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Enable and start timer hardware
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    // Step 6: Initialize Display & Run Loop
    ESP_LOGI(TAG, "Stopwatch update");

    lcd_init();   // Initialize LCD display
    watch_init(); // Initialize stopwatch face

    for (;;) {
        watch_update(timer_ticks);
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield to keep watchdog/FreeRTOS happy
    }
}
