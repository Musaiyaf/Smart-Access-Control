#include "HardwareController.h"

// ------------------------------------------------------------------
// LEDC resolution (channel assignment is automatic in Arduino-ESP32 3.x;
// the pin-based ledcAttach/ledcWrite/ledcChangeFrequency API replaced the
// channel-based ledcSetup/ledcAttachPin API used on the 2.x core.)
// ------------------------------------------------------------------
#define BUZZER_LEDC_RES      8   // 8-bit resolution (0-255)

static bool pins_initialized = false;

// ------------------------------------------------------------------
// Initialize Buzzer PWM hardware
// ------------------------------------------------------------------
static void buzzer_pwm_init(void)
{
    ledcAttach(BUZZER_PIN, 1000, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_PIN, 0);
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
    buzzer_pwm_init();
    Serial.printf("[HW] Buzzer on GPIO%d\n", BUZZER_PIN);

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

    ledcChangeFrequency(BUZZER_PIN, freq, BUZZER_LEDC_RES);
    ledcWrite(BUZZER_PIN, 128);   // 50% duty cycle

    delay(duration);

    ledcWrite(BUZZER_PIN, 0);     // Silence
}

void hw_buzzer_stop(void)
{
    if (!pins_initialized)
        return;
    ledcWrite(BUZZER_PIN, 0);
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
    ledcWrite(BUZZER_PIN, 128);

    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ_SUCCESS, BUZZER_LEDC_RES);
    delay(100);

    ledcChangeFrequency(BUZZER_PIN, 659, BUZZER_LEDC_RES);   // E5
    delay(100);

    ledcChangeFrequency(BUZZER_PIN, 784, BUZZER_LEDC_RES);   // G5
    delay(100);

    ledcChangeFrequency(BUZZER_PIN, 1047, BUZZER_LEDC_RES);  // C6
    delay(200);

    ledcWrite(BUZZER_PIN, 0);
}

void hw_buzzer_fail(void)
{
    if (!pins_initialized)
        return;

    // Descending buzz: 200Hz -> 150Hz
    ledcWrite(BUZZER_PIN, 128);

    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ_FAIL, BUZZER_LEDC_RES);
    delay(150);

    ledcChangeFrequency(BUZZER_PIN, 150, BUZZER_LEDC_RES);
    delay(150);

    ledcWrite(BUZZER_PIN, 0);
}

void hw_buzzer_lockout(void)
{
    if (!pins_initialized)
        return;

    // Slow pulsing: beep 100ms, pause 200ms, repeat 3 times
    ledcChangeFrequency(BUZZER_PIN, BUZZER_FREQ_LOCKOUT, BUZZER_LEDC_RES);

    for (uint8_t i = 0; i < 3; i++)
    {
        ledcWrite(BUZZER_PIN, 128);
        delay(100);

        ledcWrite(BUZZER_PIN, 0);
        delay(200);
    }
}
