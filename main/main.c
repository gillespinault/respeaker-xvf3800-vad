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
#include "esp_timer.h"

#include "xvf3800_i2c.h"
#include "audio_pipeline.h"

static const char *TAG = "main";

// LED Colors (RGB format for XVF3800)
// Base color stays constant blue
#define COLOR_BASE            0x0000AA    // Dark blue (constant)

// DoA indicator changes with VAD state
#define COLOR_DOA_SILENCE     0xFF6600    // Orange = silence
#define COLOR_DOA_SPEECH      0xFF0000    // Red = speech detected

// VAD state with debouncing
static bool g_is_speaking = false;
static int g_speech_counter = 0;
static int g_silence_counter = 0;

// Debounce thresholds (in callback invocations, ~30ms each)
#define SPEECH_THRESHOLD  5   // ~150ms of continuous speech to trigger
#define SILENCE_THRESHOLD 15  // ~450ms of silence to reset

/**
 * @brief Callback when VAD detects speech start
 * Uses debouncing to avoid flickering
 */
void on_speech_start(void)
{
    g_silence_counter = 0;  // Reset silence counter
    g_speech_counter++;

    // Only trigger after sustained speech
    if (!g_is_speaking && g_speech_counter >= SPEECH_THRESHOLD) {
        g_is_speaking = true;
        ESP_LOGI(TAG, ">>> SPEECH DETECTED");

        // Visual feedback: DoA indicator turns red
        xvf3800_set_led_doa_colors(COLOR_BASE, COLOR_DOA_SPEECH);
    }
}

/**
 * @brief Callback when VAD detects speech end
 * Uses debouncing to avoid flickering
 */
void on_speech_end(void)
{
    g_speech_counter = 0;  // Reset speech counter
    g_silence_counter++;

    // Only reset after sustained silence
    if (g_is_speaking && g_silence_counter >= SILENCE_THRESHOLD) {
        g_is_speaking = false;
        ESP_LOGI(TAG, "    (silence)");

        // Visual feedback: DoA indicator back to orange
        xvf3800_set_led_doa_colors(COLOR_BASE, COLOR_DOA_SILENCE);
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

    // Configure LEDs: DoA mode with blue base, orange DoA indicator (silence state)
    xvf3800_set_led_effect(LED_EFFECT_DOA);
    xvf3800_set_led_doa_colors(COLOR_BASE, COLOR_DOA_SILENCE);
    xvf3800_set_led_brightness(255);  // Full brightness

    ESP_LOGI(TAG, "XVF3800 initialized");

    // Initialize audio pipeline with VADNet
    audio_pipeline_config_t audio_cfg = {
        .on_speech_start = on_speech_start,
        .on_speech_end = on_speech_end,
    };

    ESP_ERROR_CHECK(audio_pipeline_init(&audio_cfg));

    ESP_LOGI(TAG, "Audio pipeline with VADNet started");
    ESP_LOGI(TAG, "Listening for speech... (DoA orange=silence, red=speech)");

    // Main loop - audio processing happens in dedicated tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
