#include <stdint.h>
#include "esp_adc/adc_oneshot.h"
#include "joy.h"

// Number of samples to average when establishing center position baseline
#define JOY_CENTER_SAMPLES 16

// Global handle for ADC Unit 1 peripheral
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// Measured resting center baseline values for displacement tracking
static int_fast32_t x_center = 0;
static int_fast32_t y_center = 0;

/**
 * @brief Initializes ADC Unit 1, configures channels 6 & 7, and calibrates the joystick center point.
 * @param None
 * @return int32_t ESP_OK on success, or an error code on failure.
 */
int32_t joy_init(void)
{
    // Configure and initialize ADC Unit 1 with ULP disabled
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (err != ESP_OK) {
        // Return early if unit creation failed
        return err;
    }

    // Configure Channels 6 (X-axis) and 7 (Y-axis) with default bitwidth and 12 dB attenuation
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    err = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config);
    if (err != ESP_OK) {
        // Return early if channel 6 configuration failed
        return err;
    }
    err = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &config);
    if (err != ESP_OK) {
        // Return early if channel 7 configuration failed
        return err;
    }

    // Accumulate raw readings to compute average center resting position
    int32_t sum_x = 0;
    int32_t sum_y = 0;

    // Loop through sample count to read raw ADC initial positions
    for (int32_t i = 0; i < JOY_CENTER_SAMPLES; i++) {
        int_fast32_t raw_x = 0;
        int_fast32_t raw_y = 0;

        adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, (int *)&raw_x);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, (int *)&raw_y);

        sum_x += raw_x;
        sum_y += raw_y;
    }

    x_center = sum_x / JOY_CENTER_SAMPLES;
    y_center = sum_y / JOY_CENTER_SAMPLES;

    return ESP_OK;
}

/**
 * @brief Deinitializes and deletes ADC Unit 1 handle if active.
 * @param None
 * @return int32_t ESP_OK on success, or an error code on failure.
 */
int32_t joy_deinit(void)
{
    // Clean up allocated ADC unit driver resources if initialized
    if (adc1_handle != NULL) {
        esp_err_t err = adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
        return err;
    }
    return ESP_OK;
}

/**
 * @brief Reads current joystick position and calculates X and Y displacement from center baseline.
 * @param dcx Pointer to int32_t variable where calculated X displacement will be stored.
 * @param dcy Pointer to int32_t variable where calculated Y displacement will be stored.
 * @return void
 */
void joy_get_displacement(int32_t *dcx, int32_t *dcy)
{
    int_fast32_t rx = 0;
    int_fast32_t ry = 0;

    // Perform one-shot read of X and Y channels if ADC unit handle exists
    if (adc1_handle != NULL) {
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, (int *)&rx);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, (int *)&ry);
    }

    // Compute relative displacement if output pointer for X-axis is valid
    if (dcx != NULL) {
        *dcx = (int32_t)(rx - x_center);
    }
    // Compute relative displacement if output pointer for Y-axis is valid
    if (dcy != NULL) {
        *dcy = (int32_t)(ry - y_center);
    }
}