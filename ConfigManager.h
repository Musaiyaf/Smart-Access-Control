#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum PIN length supported.
 */
#define PIN_MAX_LENGTH  4

/**
 * @brief Default PIN set at first boot.
 */
#define PIN_DEFAULT     "1234"

/**
 * @brief Initialize NVS and load configuration.
 * Must be called once before any other config functions.
 * Creates default values if NVS is empty (first boot).
 * 
 * @return true if initialization succeeded.
 */
bool config_init(void);

/**
 * @brief Get the stored PIN code.
 * 
 * @param out_buf   Output buffer (must be at least PIN_MAX_LENGTH + 1 bytes).
 * @param buf_size  Size of output buffer.
 * @return true if PIN loaded successfully.
 */
bool config_get_pin(char *out_buf, size_t buf_size);

/**
 * @brief Update the stored PIN code in NVS.
 * 
 * @param new_pin   New 4-digit PIN string.
 * @return true if saved successfully.
 */
bool config_set_pin(const char *new_pin);

/**
 * @brief Get the number of consecutive failed authentication attempts.
 * 
 * @return uint8_t Failed attempts count.
 */
uint8_t config_get_failed_attempts(void);

/**
 * @brief Set (persist) the failed attempts count.
 * 
 * @param count Number of failed attempts.
 * @return true if saved successfully.
 */
bool config_set_failed_attempts(uint8_t count);

/**
 * @brief Reset all configuration to factory defaults.
 * Clears NVS namespace and rewrites defaults.
 * 
 * @return true if reset succeeded.
 */
bool config_factory_reset(void);

/**
 * @brief Get max allowed failed attempts before lockout.
 */
uint8_t config_get_max_attempts(void);

/**
 * @brief Get lockout duration in milliseconds.
 */
uint32_t config_get_lockout_duration_ms(void);

/**
 * @brief Get grant duration in milliseconds (door open time).
 */
uint32_t config_get_grant_duration_ms(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_MANAGER_H

