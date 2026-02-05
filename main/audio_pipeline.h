/**
 * @file audio_pipeline.h
 * @brief Audio pipeline with ESP-SR VADNet
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for VAD events
 */
typedef void (*vad_callback_t)(void);

/**
 * @brief Audio pipeline configuration
 */
typedef struct {
    vad_callback_t on_speech_start;  ///< Called when speech is detected
    vad_callback_t on_speech_end;    ///< Called when speech ends
} audio_pipeline_config_t;

/**
 * @brief Initialize the audio pipeline with VADNet
 * @param config Pipeline configuration with callbacks
 * @return ESP_OK on success
 */
esp_err_t audio_pipeline_init(const audio_pipeline_config_t *config);

/**
 * @brief Deinitialize the audio pipeline
 */
void audio_pipeline_deinit(void);

/**
 * @brief Check if VAD currently detects speech
 * @return true if speech is detected
 */
bool audio_pipeline_is_speaking(void);

#ifdef __cplusplus
}
#endif
