#include "ConfigManager.h"
#include <nvs_flash.h>
#include <nvs.h>

// ------------------------------------------------------------------
// NVS Namespace & Keys
// ------------------------------------------------------------------
#define NVS_NAMESPACE       "access_ctrl"
#define NVS_KEY_PIN         "pin_code"
#define NVS_KEY_FAIL_COUNT  "fail_count"
#define NVS_KEY_INIT_FLAG   "initialized"

// ------------------------------------------------------------------
// Default constants
// ------------------------------------------------------------------
#define DEFAULT_MAX_ATTEMPTS      3
#define DEFAULT_LOCKOUT_DURATION  30000   // 30 seconds
#define DEFAULT_GRANT_DURATION    5000    // 5 seconds

// ------------------------------------------------------------------
// Initialization
// ------------------------------------------------------------------
bool config_init(void)
{
    Serial.println("[CONFIG] Initializing NVS...");

    // Initialize NVS flash
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated / needs erase
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        Serial.printf("[CONFIG] NVS init failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // Open namespace
    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        Serial.printf("[CONFIG] NVS open failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // Check if first boot (not initialized)
    uint8_t init_flag = 0;
    err = nvs_get_u8(handle, NVS_KEY_INIT_FLAG, &init_flag);
    if (err != ESP_OK || init_flag != 0x01)
    {
        Serial.println("[CONFIG] First boot — writing default configuration...");

        // Set default PIN
        nvs_set_str(handle, NVS_KEY_PIN, PIN_DEFAULT);

        // Zero out failed attempts
        nvs_set_u8(handle, NVS_KEY_FAIL_COUNT, 0);

        // Mark as initialized
        nvs_set_u8(handle, NVS_KEY_INIT_FLAG, 0x01);

        err = nvs_commit(handle);
        if (err == ESP_OK)
        {
            Serial.println("[CONFIG] Default configuration written.");
        }
    }
    else
    {
        Serial.println("[CONFIG] Configuration loaded from NVS.");
    }

    nvs_close(handle);
    return true;
}

// ------------------------------------------------------------------
// PIN Management
// ------------------------------------------------------------------
bool config_get_pin(char *out_buf, size_t buf_size)
{
    if (out_buf == nullptr || buf_size < (PIN_MAX_LENGTH + 1))
    {
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        Serial.printf("[CONFIG] Failed to open NVS for PIN read: %s\n", esp_err_to_name(err));
        return false;
    }

    size_t len = buf_size;
    err = nvs_get_str(handle, NVS_KEY_PIN, out_buf, &len);
    nvs_close(handle);

    if (err != ESP_OK)
    {
        Serial.printf("[CONFIG] PIN read failed: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool config_set_pin(const char *new_pin)
{
    // Validate: must be exactly 4 digits
    if (new_pin == nullptr || strlen(new_pin) != PIN_MAX_LENGTH)
    {
        Serial.println("[CONFIG] Invalid PIN length (must be 4 digits).");
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return false;
    }

    err = nvs_set_str(handle, NVS_KEY_PIN, new_pin);
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK)
    {
        Serial.println("[CONFIG] PIN updated successfully.");
    }
    return (err == ESP_OK);
}

// ------------------------------------------------------------------
// Failed Attempts Persistence
// ------------------------------------------------------------------
uint8_t config_get_failed_attempts(void)
{
    nvs_handle_t handle;
    uint8_t count = 0;

    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_OK)
    {
        nvs_get_u8(handle, NVS_KEY_FAIL_COUNT, &count);
        nvs_close(handle);
    }

    return count;
}

bool config_set_failed_attempts(uint8_t count)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return false;
    }

    nvs_set_u8(handle, NVS_KEY_FAIL_COUNT, count);
    err = nvs_commit(handle);
    nvs_close(handle);

    return (err == ESP_OK);
}

// ------------------------------------------------------------------
// Factory Reset
// ------------------------------------------------------------------
bool config_factory_reset(void)
{
    Serial.println("[CONFIG] Factory reset...");

    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK)
    {
        Serial.printf("[CONFIG] Erase failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // Re-initialize to write defaults
    return config_init();
}

// ------------------------------------------------------------------
// Configuration Constants
// ------------------------------------------------------------------
uint8_t config_get_max_attempts(void)
{
    return DEFAULT_MAX_ATTEMPTS;
}

uint32_t config_get_lockout_duration_ms(void)
{
    return DEFAULT_LOCKOUT_DURATION;
}

uint32_t config_get_grant_duration_ms(void)
{
    return DEFAULT_GRANT_DURATION;
}

