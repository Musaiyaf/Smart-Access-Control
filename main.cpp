#include <Arduino.h>
#include <cstring>
#include <lvgl.h>
#include <esp_display_panel.hpp>

#include "lvgl_v8_port.h"
#include "ConfigManager.h"
#include "HardwareController.h"

#include "login_gen.h"
#include "accessGranted_gen.h"
#include "systemLocked_gen.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// =====================================================
// EXTERN "C" LINKAGE for C-generated LVGL screen files
// These callbacks are referenced by login_gen.c (compiled as C)
// =====================================================

#ifdef __cplusplus
extern "C"
{
#endif

    void pin_key_event(lv_event_t *event);
    void clear_pin_event(lv_event_t *event);
    void submit_pin_event(lv_event_t *event);

#ifdef __cplusplus
}
#endif

// =====================================================
// ACCESS CONTROL CLASS
// =====================================================

class AccessControl
{
private:
    char correctPin[5];
    int failedAttempts;
    const int maxAttempts;

public:
    AccessControl()
        : failedAttempts(0), maxAttempts(config_get_max_attempts())
    {
        strcpy(correctPin, PIN_DEFAULT);
    }

    // Must be called after config_init() (NVS is not ready yet when this
    // object's constructor runs — global objects are constructed before
    // setup(), which is where config_init() happens).
    void begin()
    {
        if (!config_get_pin(correctPin, sizeof(correctPin)))
        {
            strcpy(correctPin, PIN_DEFAULT);
        }
        failedAttempts = config_get_failed_attempts();
        Serial.printf("[ACCESS] Loaded PIN from NVS. Failed attempts: %d/%d\n",
                      failedAttempts, maxAttempts);
    }

    bool authenticate(const char *pin)
    {
        if (strcmp(pin, correctPin) == 0)
        {
            failedAttempts = 0;
            config_set_failed_attempts(0);
            return true;
        }

        failedAttempts++;
        config_set_failed_attempts(failedAttempts);
        return false;
    }

    bool isLocked() const
    {
        return failedAttempts >= maxAttempts;
    }

    int attemptsRemaining() const
    {
        return maxAttempts - failedAttempts;
    }

    void reset()
    {
        failedAttempts = 0;
        config_set_failed_attempts(0);
    }

    int getFailedAttempts() const
    {
        return failedAttempts;
    }

    void changePin(const char *newPin)
    {
        if (strlen(newPin) == 4)
        {
            strcpy(correctPin, newPin);
            config_set_pin(newPin);
            Serial.println("[ACCESS] PIN changed successfully.");
        }
    }
};

// =====================================================
// STATE MACHINE & GLOBAL VARIABLES
// =====================================================

enum SystemState
{
    STATE_LOGIN,
    STATE_GRANTED,
    STATE_LOCKED
};

static SystemState currentState = STATE_LOGIN;
static uint32_t stateTimer = 0;

static AccessControl security;

static char enteredPin[5] = "";
static uint8_t pinLength = 0;

static lv_obj_t *pinLabel = nullptr;
static lv_obj_t *messageLabel = nullptr;

// =====================================================
// UPDATE PIN DISPLAY
// =====================================================

static void updatePinDisplay()
{
    if (pinLabel == nullptr)
    {
        return;
    }

    char dots[20] = "";
    for (uint8_t i = 0; i < pinLength; i++)
    {
        strcat(dots, "* ");
    }

    lv_label_set_text(pinLabel, dots);
}

// =====================================================
// INITIALIZE PIN UI
// =====================================================

void initialize_pin_ui()
{
    pinLabel = login_gen_get_pin_label();
    messageLabel = login_gen_get_message_label();

    updatePinDisplay();

    if (messageLabel != nullptr)
    {
        lv_label_set_text(messageLabel, "ENTER YOUR PIN");
    }

    // Reset PIN entry state on fresh screen load
    pinLength = 0;
    enteredPin[0] = '\0';
}

// =====================================================
// NUMBER BUTTON EVENT (C-linkage for C-compiled screen generator)
// =====================================================

extern "C" void pin_key_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (pinLength >= 4)
    {
        return;
    }

    const char *number = static_cast<const char *>(
        lv_event_get_user_data(event));

    if (number == nullptr)
    {
        return;
    }

    enteredPin[pinLength] = number[0];
    pinLength++;
    enteredPin[pinLength] = '\0';

    // Play key click sound
    hw_buzzer_click();

    updatePinDisplay();
}

// =====================================================
// CLEAR BUTTON EVENT
// =====================================================

extern "C" void clear_pin_event(lv_event_t *event)
{
    (void)event;

    pinLength = 0;
    enteredPin[0] = '\0';
    updatePinDisplay();

    hw_buzzer_click();

    if (messageLabel != nullptr)
    {
        lv_label_set_text(messageLabel, "ENTER YOUR PIN");
        lv_obj_set_style_text_color(messageLabel, lv_color_hex(0x9CA3AF), 0);
    }
}

// =====================================================
// SUBMIT BUTTON EVENT
// =====================================================

extern "C" void submit_pin_event(lv_event_t *event)
{
    (void)event;

    if (pinLength != 4)
    {
        if (messageLabel != nullptr)
        {
            lv_label_set_text(messageLabel, "PIN MUST HAVE 4 DIGITS");
            lv_obj_set_style_text_color(messageLabel, lv_color_hex(0xFF3333), 0);
        }
        hw_buzzer_fail();
        return;
    }

    bool authenticated = security.authenticate(enteredPin);

    if (authenticated)
    {
        // Unlock door
        hw_door_unlock();
        hw_buzzer_success();

        Serial.printf("[ACCESS] GRANTED! Door unlocked at %lu ms\n", millis());

        lv_obj_t *accessScreen = accessGranted_gen_create();
        lv_scr_load_anim(accessScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, true);

        currentState = STATE_GRANTED;
        stateTimer = millis();
        return;
    }

    if (security.isLocked())
    {
        // Lock the system
        hw_door_lock();
        hw_buzzer_lockout();

        Serial.printf("[ACCESS] SYSTEM LOCKED! Too many failed attempts.\n");

        lv_obj_t *lockedScreen = systemLocked_gen_create();
        lv_scr_load_anim(lockedScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, true);

        currentState = STATE_LOCKED;
        stateTimer = millis();
        return;
    }

    // Authentication failed but not yet locked out
    hw_buzzer_fail();
    Serial.printf("[ACCESS] DENIED! Attempts left: %d\n", security.attemptsRemaining());

    if (messageLabel != nullptr)
    {
        lv_label_set_text_fmt(
            messageLabel,
            "ACCESS DENIED\nATTEMPTS LEFT: %d",
            security.attemptsRemaining());
        lv_obj_set_style_text_color(messageLabel, lv_color_hex(0xFF3333), 0);
    }

    pinLength = 0;
    enteredPin[0] = '\0';
    updatePinDisplay();
}

// =====================================================
// LVGL INTEGRATION & ARDUINO LIFE CYCLE
// =====================================================

void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("===========================================");
    Serial.println("  SMART ACCESS CONTROL v2.0");
    Serial.println("  ESP32-S3 + VIEWE UEDX24320028E-WB-A");
    Serial.println("  (GC9307 display + CHSC6540 touch)");
    Serial.println("===========================================");

    // ---- Initialize hardware modules ----
    Serial.println("[INIT] Starting hardware initialization...");

    // 1. NVS Configuration (must be first — others may depend on it)
    if (!config_init())
    {
        Serial.println("[FATAL] NVS initialization failed!");
    }
    security.begin();

    // 2. Display + touch panel, via VIEWE's official board profile
    Serial.println("[INIT] Initializing display panel board...");
    Board *board = new Board();
    board->init();
    assert(board->begin());

    // 3. LVGL library + display/touch drivers (handled by the port layer,
    //    including the tick source and the background refresh task)
    Serial.println("[INIT] Initializing LVGL...");
    lvgl_port_init(board->getLCD(), board->getTouch());

    // 4. Hardware peripherals (relay, buzzer)
    hw_init();

    // ---- Load and show login screen ----
    Serial.println("[INIT] Creating UI...");
    lvgl_port_lock(-1);
    lv_obj_t *loginScreen = login_gen_create();
    lv_scr_load(loginScreen);
    initialize_pin_ui();
    lvgl_port_unlock();

    currentState = STATE_LOGIN;

    Serial.println("[INIT] Smart Access Control ready.");
    Serial.printf("[INIT] PIN from NVS: %s | Failed: %d/%d\n",
                  "****", security.getFailedAttempts(), config_get_max_attempts());
    Serial.println("===========================================");
}

void loop()
{
    // LVGL timer handling and screen refresh run on a background task
    // started by lvgl_port_init(); this loop only drives the access-control
    // state machine, which must take the LVGL lock before touching any
    // LVGL objects since it runs on a different task than that refresh loop.

    if (currentState == STATE_GRANTED && (millis() - stateTimer) > config_get_grant_duration_ms())
    {
        // Re-lock door and return to login screen after configured duration
        hw_door_lock();
        Serial.println("[STATE] Grant timeout — relocking door.");

        lvgl_port_lock(-1);
        lv_obj_t *loginScreen = login_gen_create();
        lv_scr_load_anim(loginScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, true);
        initialize_pin_ui();
        lvgl_port_unlock();
        currentState = STATE_LOGIN;
    }
    else if (currentState == STATE_LOCKED && (millis() - stateTimer) > config_get_lockout_duration_ms())
    {
        // Reset security state and return to login after lockout duration
        security.reset();
        Serial.println("[STATE] Lockout timeout — resetting and returning to login.");

        lvgl_port_lock(-1);
        lv_obj_t *loginScreen = login_gen_create();
        lv_scr_load_anim(loginScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, true);
        initialize_pin_ui();
        lvgl_port_unlock();
        currentState = STATE_LOGIN;
    }

    delay(20);
}
