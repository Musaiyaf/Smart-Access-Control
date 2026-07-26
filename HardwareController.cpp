#include "HardwareController.h"

// ------------------------------------------------------------------
// LEDC channel assignment (separate from GPIO pin number)
// ------------------------------------------------------------------
#define BUZZER_LEDC_CHANNEL  0
#define BUZZER_LEDC_RES      8   // 8-bit resolution (0-255)

static bool pins_initialized = false;

// ------------------------------------------------------------------
// Initialize Buzzer PWM hardware
// Note: Legacy LEDC API (ledcSetup/ledcAttachPin) is used for
// compatibility with both ESP32 Arduino Core v2.x and v3.x.
// ------------------------------------------------------------------
static void buzzer_pwm_init(void)
{
    ledcSetup(BUZZER_LEDC_CHANNEL, 1000, BUZZER_LEDC_RES);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

// ------------------------------------------------------------------
// Initialization
// ------------------------------------------------------------------
void hw_init(void)
{
    Serial.println("[HW] Initializing hardware peripherals...");

    // --- Door Relay ---
    pinMode(DOOR_RELAY, OUTPUT);
    digitalWrite(DOOR_RELAY, LOW); // Start locked
    Serial.printf("[HW] Door relay on GPIO%d (LOW = locked)\n", DOOR_RELAY);

    // --- Buzzer (PWM via ledc) ---
    pinMode(BUZZER_PIN, OUTPUT);
    buzzer_pwm_init();
    Serial.printf("[HW] Buzzer on GPIO%d (LEDC ch%d)\n", BUZZER_PIN, BUZZER_LEDC_CHANNEL);

    pins_initialized = true;
    Serial.println("[HW] Hardware initialization complete.");
}

// ------------------------------------------------------------------
// Door Control
// ------------------------------------------------------------------
void hw_door_unlock(void)
{
    if (!pins_initialized)
        return;
    digitalWrite(DOOR_RELAY, HIGH);
    Serial.println("[HW] Door UNLOCKED");
}

void hw_door_lock(void)
{
    if (!pins_initialized)
        return;
    digitalWrite(DOOR_RELAY, LOW);
    Serial.println("[HW] Door LOCKED");
}

bool hw_door_toggle(void)
{
    if (!pins_initialized)
        return false;
    bool state = !digitalRead(DOOR_RELAY);
    digitalWrite(DOOR_RELAY, state ? HIGH : LOW);
    Serial.printf("[HW] Door toggled: %s\n", state ? "UNLOCKED" : "LOCKED");
    return state;
}

bool hw_door_is_unlocked(void)
{
    if (!pins_initialized)
        return false;
    return (digitalRead(DOOR_RELAY) == HIGH);
}

// ------------------------------------------------------------------
// Buzzer Primitives
// ------------------------------------------------------------------
void hw_buzzer_beep(uint16_t freq, uint16_t duration)
{
    if (!pins_initialized)
        return;

    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, freq, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_LEDC_CHANNEL, 128);   // 50% duty cycle

    delay(duration);

    ledcWrite(BUZZER_LEDC_CHANNEL, 0);     // Silence
}

void hw_buzzer_stop(void)
{
    if (!pins_initialized)
        return;
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

// ------------------------------------------------------------------
// Buzzer Patterns
// ------------------------------------------------------------------
void hw_buzzer_click(void)
{
    hw_buzzer_beep(BUZZER_FREQ_CLICK, 30);
}

void hw_buzzer_success(void)
{
    if (!pins_initialized)
        return;

    // Ascending chime: C5 -> E5 -> G5 -> C6
    ledcWrite(BUZZER_LEDC_CHANNEL, 128);

    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, BUZZER_FREQ_SUCCESS, BUZZER_LEDC_RES);
    delay(100);

    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, 659, BUZZER_LEDC_RES);   // E5
    delay(100);

    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, 784, BUZZER_LEDC_RES);   // G5
    delay(100);

    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, 1047, BUZZER_LEDC_RES);  // C6
    delay(200);

    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

void hw_buzzer_fail(void)
{
    if (!pins_initialized)
        return;

    // Descending buzz: 200Hz -> 150Hz
    ledcWrite(BUZZER_LEDC_CHANNEL, 128);

    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, BUZZER_FREQ_FAIL, BUZZER_LEDC_RES);
    delay(150);

    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, 150, BUZZER_LEDC_RES);
    delay(150);

    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

void hw_buzzer_lockout(void)
{
    if (!pins_initialized)
        return;

    // Slow pulsing: beep 100ms, pause 200ms, repeat 3 times
    ledcChangeFrequency(BUZZER_LEDC_CHANNEL, BUZZER_FREQ_LOCKOUT, BUZZER_LEDC_RES);

    for (uint8_t i = 0; i < 3; i++)
    {
        ledcWrite(BUZZER_LEDC_CHANNEL, 128);
        delay(100);

        ledcWrite(BUZZER_LEDC_CHANNEL, 0);
        delay(200);
    }
}

