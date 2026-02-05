/**
 * @file xvf3800_i2c.h
 * @brief XVF3800 I2C control interface
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED Effects
typedef enum {
    LED_EFFECT_OFF = 0,
    LED_EFFECT_BREATH = 1,
    LED_EFFECT_RAINBOW = 2,
    LED_EFFECT_SINGLE = 3,
    LED_EFFECT_DOA = 4,
} led_effect_t;

/**
 * @brief Initialize I2C communication with XVF3800
 * @return ESP_OK on success
 */
esp_err_t xvf3800_i2c_init(void);

/**
 * @brief Get XVF3800 firmware version
 * @param version Array of 3 bytes [major, minor, patch]
 * @return ESP_OK on success
 */
esp_err_t xvf3800_get_version(uint8_t version[3]);

/**
 * @brief Set LED effect mode
 * @param effect LED effect type
 * @return ESP_OK on success
 */
esp_err_t xvf3800_set_led_effect(led_effect_t effect);

/**
 * @brief Set LED brightness
 * @param brightness 0-255
 * @return ESP_OK on success
 */
esp_err_t xvf3800_set_led_brightness(uint8_t brightness);

/**
 * @brief Set LED color (for breath/single modes)
 * @param color RGB color (0xRRGGBB)
 * @return ESP_OK on success
 */
esp_err_t xvf3800_set_led_color(uint32_t color);

/**
 * @brief Set DoA mode colors
 * @param base_color Base LED color (RGB)
 * @param doa_color Direction indicator color (RGB)
 * @return ESP_OK on success
 */
esp_err_t xvf3800_set_led_doa_colors(uint32_t base_color, uint32_t doa_color);

/**
 * @brief Enable/disable amplifier
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t xvf3800_enable_amplifier(bool enable);

/**
 * @brief Read speech energy values from XVF3800
 * @param energy Array of 4 floats for beam energies
 * @return ESP_OK on success
 */
esp_err_t xvf3800_get_speech_energy(float energy[4]);

/**
 * @brief Save current configuration to XVF3800 flash
 * @return ESP_OK on success
 */
esp_err_t xvf3800_save_configuration(void);

#ifdef __cplusplus
}
#endif
