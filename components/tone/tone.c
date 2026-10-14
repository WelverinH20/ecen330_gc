#include <stdlib.h>
#include <math.h>
#include "tone.h"

#define TONE_BIAS_LEVEL           0x80
#define TONE_AMPLITUDE_SCALE      127.0f
#define TONE_MAX_UINT8_FLOAT      255.0f
#define TONE_QUARTER_DIVISOR      4.0f
#define TONE_THREE_QUARTER_FACTOR 3.0f
#define TONE_MAX_UINT8_HEX        0xFF

static uint8_t *tone_buffer = NULL;
static uint32_t s_sample_rate = 0;
static size_t s_max_buffer_size = 0;

/**
 * @brief Initialize the tone driver and allocate the waveform buffer.
 * @param sample_hz Sample rate in Hz to playback tone.
 * @return Returns 0 if successful, or non-zero otherwise.
 */
int32_t tone_init(uint32_t sample_hz) {
    // Check sample rate parameter to make sure it is at least twice the lowest Nyquist frequency
    if (sample_hz < (2U * LOWEST_FREQ)) {
        return -1;
    }

    s_sample_rate = sample_hz;

    // Allocate a global buffer sized for the lowest allowed frequency (20 Hz)
    s_max_buffer_size = (size_t)(s_sample_rate / LOWEST_FREQ);
    tone_buffer = (uint8_t *)malloc(s_max_buffer_size);
    // Check if memory allocation was successful
    if (tone_buffer == NULL) {
        return -1;
    }

    // Call sound_init() from the sound component
    int32_t ret = sound_init(sample_hz);
    // Check if sound initialization failed and cleanup if needed
    if (ret != 0) {
        free(tone_buffer);
        tone_buffer = NULL;
        return ret;
    }

    return 0;
}

/**
 * @brief Free resources used for tone generation (DAC, etc.).
 * @return Returns 0 if successful, or non-zero otherwise.
 */
int32_t tone_deinit(void) {
    // Check if tone buffer is allocated before freeing
    if (tone_buffer != NULL) {
        free(tone_buffer);
        tone_buffer = NULL;
    }

    return sound_deinit();
}

/**
 * @brief Start playing the specified tone type at a given frequency.
 * @param tone One of the enumerated tone types.
 * @param freq Frequency of the tone in Hz.
 */
void tone_start(tone_t tone, uint32_t freq) {
    // Validate frequency, sample rate, and buffer existence
    if (freq < LOWEST_FREQ || s_sample_rate == 0 || tone_buffer == NULL) {
        return;
    }

    // Calculate the number of samples in one period for the specified frequency
    uint32_t num_samples = s_sample_rate / freq;
    // Ensure calculated samples do not exceed max buffer size
    if (num_samples > s_max_buffer_size) {
        num_samples = s_max_buffer_size;
    }

    // Generate one cycle of the specified waveform into the allocated buffer
    for (uint32_t i = 0; i < num_samples; i++) {
        float x = (float)i;
        float val = 0.0f;

        // Switch based on tone waveform type
        switch (tone) {
            case SINE_T: {
                // Calculate sine wave sample value
                val = (float)TONE_BIAS_LEVEL + TONE_AMPLITUDE_SCALE * sinf(2.0f * (float)M_PI * x / (float)num_samples);
                break;
            }
            case SQUARE_T: {
                // Generate square wave high or low depending on period half
                if (i < num_samples / 2) {
                    val = TONE_MAX_UINT8_FLOAT;
                } else {
                    val = 0.0f;
                }
                break;
            }
            case TRIANGLE_T: {
                // Calculate linear segments for triangle wave
                float quarter = (float)num_samples / TONE_QUARTER_DIVISOR;
                float three_quarter = TONE_THREE_QUARTER_FACTOR * (float)num_samples / TONE_QUARTER_DIVISOR;

                if (x < quarter) {
                    // First quarter segment rising to max
                    float m = ((float)TONE_MAX_UINT8_HEX - (float)TONE_BIAS_LEVEL) / quarter;
                    float b = (float)TONE_BIAS_LEVEL;
                    val = m * x + b;
                } else if (x < three_quarter) {
                    // Middle segment falling from max to min
                    float m = (0.0f - (float)TONE_MAX_UINT8_HEX) / (three_quarter - quarter);
                    float b = (float)TONE_MAX_UINT8_HEX - m * quarter;
                    val = m * x + b;
                } else {
                    // Final segment rising back to bias level
                    float m = ((float)TONE_BIAS_LEVEL - 0.0f) / ((float)num_samples - three_quarter);
                    float b = 0.0f - m * three_quarter;
                    val = m * x + b;
                }
                break;
            }
            case SAW_T: {
                // Calculate linear segments for sawtooth wave
                float half = (float)num_samples / 2.0f;

                if (x < half) {
                    // First half segment rising to max
                    float m = ((float)TONE_MAX_UINT8_HEX - (float)TONE_BIAS_LEVEL) / half;
                    float b = (float)TONE_BIAS_LEVEL;
                    val = m * x + b;
                } else {
                    // Second half segment dropping to min and rising back
                    float m = ((float)TONE_BIAS_LEVEL - 0.0f) / ((float)num_samples - half);
                    float b = 0.0f - m * half;
                    val = m * x + b;
                }
                break;
            }
            case LAST_T:
            default: {
                // Default fallback to bias level
                val = (float)TONE_BIAS_LEVEL;
                break;
            }
        }

        // Clamp values to valid 8-bit unsigned range
        if (val < 0.0f) {
            val = 0.0f;
        }
        if (val > TONE_MAX_UINT8_FLOAT) {
            val = TONE_MAX_UINT8_FLOAT;
        }

        tone_buffer[i] = (uint8_t)val;
    }

    // Call sound_cyclic() to play the generated waveform buffer
    sound_cyclic(tone_buffer, num_samples);
}