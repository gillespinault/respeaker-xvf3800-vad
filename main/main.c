/**
 * @file main.c
 * @brief ReSpeaker XVF3800 VAD - Main application
 *
 * Uses ESP-SR VADNet for high-quality voice activity detection
 * with visual feedback on the XVF3800 LED ring.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "xvf3800_i2c.h"
#include "audio_pipeline.h"

static const char *TAG = "main";

// LED Colors (RGB format for XVF3800)
#define COLOR_BLUE    0x0000FF
#define COLOR_WHITE   0xFFFFFF
#define COLOR_ORANGE  0xFF8800

// VAD state
static bool g_is_speaking = false;

/**
 * @brief Callback when VAD detects speech start
 */
void on_speech_start(void)
{
    if (!g_is_speaking) {
        g_is_speaking = true;
        ESP_LOGI(TAG, ">>> SPEECH DETECTED");

        // Change LED base color to white (keep DoA indicator orange)
        xvf3800_set_led_doa_colors(COLOR_WHITE, COLOR_ORANGE);
    }
}

/**
 * @brief Callback when VAD detects speech end
 */
void on_speech_end(void)
{
    if (g_is_speaking) {
        g_is_speaking = false;
        ESP_LOGI(TAG, "    (silence)");

        // Return LED base color to blue
        xvf3800_set_led_doa_colors(COLOR_BLUE, COLOR_ORANGE);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ReSpeaker XVF3800 VAD");
    ESP_LOGI(TAG, "  ESP-SR VADNet + XMOS XVF3800");
    ESP_LOGI(TAG, "========================================");

    // Initialize I2C for XVF3800 control
    ESP_ERROR_CHECK(xvf3800_i2c_init());

    // Read and display XVF3800 firmware version
    uint8_t version[3];
    if (xvf3800_get_version(version) == ESP_OK) {
        ESP_LOGI(TAG, "XVF3800 Firmware: %d.%d.%d", version[0], version[1], version[2]);
    }

    // Configure LEDs: DoA mode with blue base, orange direction
    xvf3800_set_led_effect(LED_EFFECT_DOA);
    xvf3800_set_led_brightness(255);
    xvf3800_set_led_doa_colors(COLOR_BLUE, COLOR_ORANGE);

    // Enable amplifier
    xvf3800_enable_amplifier(true);

    ESP_LOGI(TAG, "XVF3800 initialized");

    // Initialize audio pipeline with VADNet
    audio_pipeline_config_t audio_cfg = {
        .on_speech_start = on_speech_start,
        .on_speech_end = on_speech_end,
    };

    ESP_ERROR_CHECK(audio_pipeline_init(&audio_cfg));

    ESP_LOGI(TAG, "Audio pipeline with VADNet started");
    ESP_LOGI(TAG, "Listening for speech...");

    // Main loop - audio processing happens in dedicated tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
