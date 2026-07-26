#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief GPIO pin definitions for hardware peripherals.
 * These can be overridden via build_flags in platformio.ini.
 */
#ifndef DOOR_RELAY
#define DOOR_RELAY 5
#endif
#ifndef BUZZER_PIN
#define BUZZER_PIN 4
#endif

/**
 * @brief Buzzer tone frequencies (Hz).
 */
#define BUZZER_FREQ_CLICK 1800
#define BUZZER_FREQ_SUCCESS 523 // C5
#define BUZZER_FREQ_FAIL 200
#define BUZZER_FREQ_LOCKOUT 100

    /**
     * @brief Initialize all hardware GPIO peripherals.
     * Configures relay pin as OUTPUT, buzzer pin as OUTPUT with PWM.
     */
    void hw_init(void);

    /**
     * @brief Unlock the door (relay active).
     */
    void hw_door_unlock(void);

    /**
     * @brief Lock the door (relay inactive).
     */
    void hw_door_lock(void);

    /**
     * @brief Toggle door state (used for testing).
     * @return true if now unlocked, false if locked.
     */
    bool hw_door_toggle(void);

    /**
     * @brief Get current door state.
     * @return true if door is unlocked.
     */
    bool hw_door_is_unlocked(void);

    /**
     * @brief Play a short key-click beep.
     */
    void hw_buzzer_click(void);

    /**
     * @brief Play a success chime (ascending tone).
     */
    void hw_buzzer_success(void);

    /**
     * @brief Play a failure tone (descending buzz).
     */
    void hw_buzzer_fail(void);

    /**
     * @brief Play a lockout alarm (slow pulsing tone).
     */
    void hw_buzzer_lockout(void);

    /**
     * @brief Beep at a specific frequency for a duration.
     *
     * @param freq      Tone frequency in Hz.
     * @param duration  Duration in milliseconds.
     */
    void hw_buzzer_beep(uint16_t freq, uint16_t duration);

    /**
     * @brief Stop any active buzzer sound.
     */
    void hw_buzzer_stop(void);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_CONTROLLER_H
