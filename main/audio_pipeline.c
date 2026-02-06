/**
 * @file audio_pipeline.c
 * @brief Audio pipeline with ESP-SR VADNet implementation
 *
 * This module:
 * 1. Receives audio from XVF3800 via I2S
 * 2. Feeds it to ESP-SR AFE with VADNet
 * 3. Triggers callbacks on speech start/end
 */

#include "audio_pipeline.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include <string.h>

// ESP-SR includes
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"

static const char *TAG = "audio_pipeline";

// I2S Configuration (RX only - audio from XVF3800)
#define I2S_NUM             I2S_NUM_1
#define I2S_BCK_PIN         8
#define I2S_WS_PIN          7
#define I2S_DIN_PIN         43
#define I2S_SAMPLE_RATE     16000

// Task configuration
#define FEED_TASK_STACK     4096
#define DETECT_TASK_STACK   8192
#define FEED_TASK_PRIO      5
#define DETECT_TASK_PRIO    5

// Module state
static i2s_chan_handle_t s_i2s_rx_handle = NULL;
static esp_afe_sr_iface_t *s_afe_handle = NULL;
static esp_afe_sr_data_t *s_afe_data = NULL;
static TaskHandle_t s_feed_task = NULL;
static TaskHandle_t s_detect_task = NULL;
static bool s_running = false;
static audio_pipeline_config_t s_config = {0};

/**
 * @brief Initialize I2S for RX only (audio from XVF3800)
 */
static esp_err_t init_i2s(void)
{
    // Create RX channel only
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &s_i2s_rx_handle));

    // Configure RX for 32-bit stereo (matching XVF3800 output)
    i2s_std_config_t rx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DIN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_i2s_rx_handle, &rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_i2s_rx_handle));

    ESP_LOGI(TAG, "I2S RX initialized (BCK=%d, WS=%d, DIN=%d)", I2S_BCK_PIN, I2S_WS_PIN, I2S_DIN_PIN);
    return ESP_OK;
}

/**
 * @brief Initialize ESP-SR AFE with VAD
 */
static esp_err_t init_afe(void)
{
    // Get AFE handle
    s_afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;

    // Configure AFE
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();

    // Audio input configuration
    // XVF3800 provides 2 channels (processed stereo from beamforming)
    afe_config.pcm_config.total_ch_num = 2;
    afe_config.pcm_config.mic_num = 2;
    afe_config.pcm_config.ref_num = 0;  // No reference (AEC done by XVF3800)
    afe_config.pcm_config.sample_rate = I2S_SAMPLE_RATE;

    // VAD configuration
    afe_config.vad_init = true;
    afe_config.vad_mode = VAD_MODE_3;  // Less sensitive (0=most sensitive, 4=least)

    // Disable features already handled by XVF3800
    afe_config.aec_init = false;  // AEC done by XVF3800
    afe_config.se_init = false;   // Noise suppression done by XVF3800
    afe_config.wakenet_init = false;  // We only want VAD
    afe_config.wakenet_model_name = NULL;

    // Memory allocation mode
    afe_config.memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    afe_config.afe_mode = SR_MODE_LOW_COST;

    // Create AFE instance
    s_afe_data = s_afe_handle->create_from_config(&afe_config);
    if (s_afe_data == NULL) {
        ESP_LOGE(TAG, "Failed to create AFE instance");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ESP-SR AFE with VAD initialized");
    return ESP_OK;
}

/**
 * @brief Task to feed audio data to AFE
 */
static void audio_feed_task(void *arg)
{
    int feed_chunksize = s_afe_handle->get_feed_chunksize(s_afe_data);
    int feed_channel = s_afe_handle->get_total_channel_num(s_afe_data);

    int16_t *feed_buffer = heap_caps_malloc(feed_chunksize * feed_channel * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    int32_t *i2s_buffer = heap_caps_malloc(feed_chunksize * 2 * sizeof(int32_t), MALLOC_CAP_INTERNAL);

    if (!feed_buffer || !i2s_buffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffers");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Feed task started (chunksize=%d, channels=%d)", feed_chunksize, feed_channel);

    size_t bytes_read;
    int debug_counter = 0;
    int32_t max_sample = 0;
    while (s_running) {
        // Read from I2S (32-bit stereo)
        esp_err_t ret = i2s_channel_read(s_i2s_rx_handle, i2s_buffer,
                                          feed_chunksize * 2 * sizeof(int32_t),
                                          &bytes_read, pdMS_TO_TICKS(100));

        if (ret != ESP_OK || bytes_read == 0) {
            if (debug_counter++ % 100 == 0) {
                ESP_LOGW(TAG, "I2S read failed or empty: ret=%d, bytes=%d", ret, bytes_read);
            }
            continue;
        }

        // Debug: check audio levels every ~3 seconds
        for (int i = 0; i < bytes_read / sizeof(int32_t); i++) {
            int32_t abs_val = i2s_buffer[i] < 0 ? -i2s_buffer[i] : i2s_buffer[i];
            if (abs_val > max_sample) max_sample = abs_val;
        }
        if (debug_counter++ % 100 == 0) {
            ESP_LOGI(TAG, "Audio level: max=%ld (bytes=%d)", (long)max_sample, bytes_read);
            max_sample = 0;
        }

        // Convert 32-bit to 16-bit (take upper 16 bits)
        int samples = bytes_read / sizeof(int32_t);
        for (int i = 0; i < samples && i < feed_chunksize * feed_channel; i++) {
            feed_buffer[i] = (int16_t)(i2s_buffer[i] >> 16);
        }

        // Feed to AFE
        s_afe_handle->feed(s_afe_data, feed_buffer);
    }

    free(feed_buffer);
    free(i2s_buffer);
    vTaskDelete(NULL);
}

/**
 * @brief Task to detect VAD results from AFE
 */
static void vad_detect_task(void *arg)
{
    ESP_LOGI(TAG, "VAD detect task started");
    int fetch_count = 0;

    while (s_running) {
        afe_fetch_result_t *result = s_afe_handle->fetch(s_afe_data);

        if (result == NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        fetch_count++;
        // Debug VAD state periodically
        if (fetch_count % 100 == 0) {
            ESP_LOGI(TAG, "VAD state: %d (fetch #%d)", result->vad_state, fetch_count);
        }

        // Check VAD state - call callbacks continuously for debouncing in main
        bool speech_detected = (result->vad_state == AFE_VAD_SPEECH);

        if (speech_detected) {
            if (s_config.on_speech_start) {
                s_config.on_speech_start();
            }
        } else {
            if (s_config.on_speech_end) {
                s_config.on_speech_end();
            }
        }
    }

    vTaskDelete(NULL);
}

esp_err_t audio_pipeline_init(const audio_pipeline_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_config = *config;

    // Initialize I2S
    esp_err_t ret = init_i2s();
    if (ret != ESP_OK) {
        return ret;
    }

    // Initialize AFE with VADNet
    ret = init_afe();
    if (ret != ESP_OK) {
        return ret;
    }

    // Start processing tasks
    s_running = true;

    xTaskCreate(audio_feed_task, "audio_feed", FEED_TASK_STACK, NULL, FEED_TASK_PRIO, &s_feed_task);
    xTaskCreate(vad_detect_task, "vad_detect", DETECT_TASK_STACK, NULL, DETECT_TASK_PRIO, &s_detect_task);

    return ESP_OK;
}

void audio_pipeline_deinit(void)
{
    s_running = false;

    if (s_feed_task) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (s_afe_data) {
        s_afe_handle->destroy(s_afe_data);
        s_afe_data = NULL;
    }

    if (s_i2s_rx_handle) {
        i2s_channel_disable(s_i2s_rx_handle);
        i2s_del_channel(s_i2s_rx_handle);
        s_i2s_rx_handle = NULL;
    }
}
