/**
 * @file xvf3800_i2c.c
 * @brief XVF3800 I2C control implementation
 */

#include "xvf3800_i2c.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "xvf3800_i2c";

// I2C Configuration
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA      5
#define I2C_MASTER_SCL      6
#define I2C_MASTER_FREQ_HZ  100000
#define I2C_TIMEOUT_MS      100

// XVF3800 I2C Address
#define XMOS_ADDR           0x2C

// Resource IDs
#define RESID_GPO           20
#define RESID_AEC           33
#define RESID_DEVICE        48

// Command IDs
#define CMD_VERSION         0
#define CMD_GPO_WRITE       1
#define CMD_SAVE_CONFIG     3
#define CMD_LED_EFFECT      12
#define CMD_LED_BRIGHTNESS  13
#define CMD_LED_SPEED       15
#define CMD_LED_COLOR       16
#define CMD_LED_DOA_COLOR   17
#define CMD_AEC_SPENERGY    74

// GPIO Pins
#define GPIO_AMP_ENABLE     31

/**
 * @brief Write bytes to XVF3800 via I2C
 */
static esp_err_t xvf3800_write(uint8_t resid, uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uint8_t buf[32];
    buf[0] = resid;
    buf[1] = cmd;
    buf[2] = len;
    if (len > 0 && data != NULL) {
        memcpy(&buf[3], data, len);
    }

    return i2c_master_write_to_device(
        I2C_MASTER_NUM,
        XMOS_ADDR,
        buf,
        3 + len,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );
}

/**
 * @brief Read bytes from XVF3800 via I2C
 */
static esp_err_t xvf3800_read(uint8_t resid, uint8_t cmd, uint8_t *data, uint8_t len)
{
    // Send read command
    uint8_t write_buf[3] = { resid, cmd, 0 };
    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM,
        XMOS_ADDR,
        write_buf,
        3,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    if (ret != ESP_OK) {
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(5));

    // Read response (status byte + data)
    uint8_t read_buf[32];
    ret = i2c_master_read_from_device(
        I2C_MASTER_NUM,
        XMOS_ADDR,
        read_buf,
        1 + len,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    if (ret == ESP_OK && data != NULL) {
        memcpy(data, &read_buf[1], len);
    }

    return ret;
}

esp_err_t xvf3800_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C initialized (SDA=%d, SCL=%d)", I2C_MASTER_SDA, I2C_MASTER_SCL);
    return ESP_OK;
}

esp_err_t xvf3800_get_version(uint8_t version[3])
{
    return xvf3800_read(RESID_DEVICE, CMD_VERSION, version, 3);
}

esp_err_t xvf3800_set_led_effect(led_effect_t effect)
{
    uint8_t data = (uint8_t)effect;
    return xvf3800_write(RESID_GPO, CMD_LED_EFFECT, &data, 1);
}

esp_err_t xvf3800_set_led_brightness(uint8_t brightness)
{
    return xvf3800_write(RESID_GPO, CMD_LED_BRIGHTNESS, &brightness, 1);
}

esp_err_t xvf3800_set_led_color(uint32_t color)
{
    uint8_t data[4] = {
        (uint8_t)(color & 0xFF),         // Blue
        (uint8_t)((color >> 8) & 0xFF),  // Green
        (uint8_t)((color >> 16) & 0xFF), // Red
        0x00
    };
    return xvf3800_write(RESID_GPO, CMD_LED_COLOR, data, 4);
}

esp_err_t xvf3800_set_led_doa_colors(uint32_t base_color, uint32_t doa_color)
{
    uint8_t data[8] = {
        // Base color (BGR little-endian)
        (uint8_t)(base_color & 0xFF),
        (uint8_t)((base_color >> 8) & 0xFF),
        (uint8_t)((base_color >> 16) & 0xFF),
        0x00,
        // DoA color (BGR little-endian)
        (uint8_t)(doa_color & 0xFF),
        (uint8_t)((doa_color >> 8) & 0xFF),
        (uint8_t)((doa_color >> 16) & 0xFF),
        0x00
    };
    return xvf3800_write(RESID_GPO, CMD_LED_DOA_COLOR, data, 8);
}

esp_err_t xvf3800_enable_amplifier(bool enable)
{
    uint8_t data[2] = { GPIO_AMP_ENABLE, enable ? 0 : 1 };  // LOW = enabled
    return xvf3800_write(RESID_GPO, CMD_GPO_WRITE, data, 2);
}

esp_err_t xvf3800_get_speech_energy(float energy[4])
{
    uint8_t data[16];
    esp_err_t ret = xvf3800_read(RESID_AEC, CMD_AEC_SPENERGY, data, 16);

    if (ret == ESP_OK) {
        for (int i = 0; i < 4; i++) {
            memcpy(&energy[i], &data[i * 4], sizeof(float));
        }
    }

    return ret;
}

esp_err_t xvf3800_save_configuration(void)
{
    uint8_t data = 1;
    esp_err_t ret = xvf3800_write(RESID_DEVICE, CMD_SAVE_CONFIG, &data, 1);
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for flash write
    return ret;
}
