#include <Arduino.h>
#include <cstring>
#include <lvgl.h>

#include "DisplayDriver.h"
#include "TouchDriver.h"
#include "ConfigManager.h"
#include "HardwareController.h"

#include "login_gen.h"
#include "accessGranted_gen.h"
#include "systemLocked_gen.h"

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
        // Load PIN from NVS
        if (!config_get_pin(correctPin, sizeof(correctPin)))
        {
            strcpy(correctPin, PIN_DEFAULT);
        }
        // Load persisted failed attempts
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
// HELPER: Recursive child lookup by name
// LVGL v9 removed lv_obj_find_by_name(), so we implement
// our own recursive search through the object tree.
// =====================================================

static lv_obj_t *find_child_by_name(lv_obj_t *parent, const char *name)
{
    if (parent == nullptr || name == nullptr)
    {
        return nullptr;
    }

    const char *obj_name = lv_obj_get_name(parent);
    if (obj_name != nullptr && strcmp(obj_name, name) == 0)
    {
        return parent;
    }

    uint32_t child_count = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < child_count; i++)
    {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        lv_obj_t *found = find_child_by_name(child, name);
        if (found != nullptr)
        {
            return found;
        }
    }

    return nullptr;
}

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
    lv_obj_t *screen = lv_screen_active();

    pinLabel = find_child_by_name(screen, "pin_label");
    messageLabel = find_child_by_name(screen, "message_label");

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
        lv_screen_load_anim(accessScreen, LV_SCREEN_LOAD_ANIM_FADE_IN, 300, 0, true);

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
        lv_screen_load_anim(lockedScreen, LV_SCREEN_LOAD_ANIM_FADE_IN, 300, 0, true);

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

static uint32_t my_tick_get_cb(void)
{
    return millis();
}

void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("===========================================");
    Serial.println("  SMART ACCESS CONTROL v2.0");
    Serial.println("  ESP32-S3 + ILI9341 + CST816S");
    Serial.println("===========================================");

    // ---- Initialize hardware modules ----
    Serial.println("[INIT] Starting hardware initialization...");

    // 1. NVS Configuration (must be first — others may depend on it)
    if (!config_init())
    {
        Serial.println("[FATAL] NVS initialization failed!");
    }

    // 2. Display (ILI9341 via TFT_eSPI)
    display_init();

    // 3. LVGL library
    lv_init();
    lv_tick_set_cb(my_tick_get_cb);

    // 4. Register LVGL display driver
    lv_display_t *disp = lv_display_create(320, 240);
    if (disp == nullptr)
    {
        Serial.println("[FATAL] lv_display_create() returned NULL!");
        while (1)
        {
            delay(1000);
        }
    }
    lv_display_set_flush_cb(disp, display_flush_cb);

    // Single frame buffer for partial rendering (saves ~280KB DRAM on no-PSRAM board)
    // 320px * 50 rows * 2 bytes per pixel (RGB565) = ~32KB draw buffer
    static uint8_t buf1[320 * 50 * 2];
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 5. Touch (CST816S capacitive via I2C)
    touch_init();
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    // 6. Hardware peripherals (relay, buzzer)
    hw_init();

    // ---- Load and show login screen ----
    lv_obj_t *loginScreen = login_gen_create();
    lv_screen_load(loginScreen);
    initialize_pin_ui();

    currentState = STATE_LOGIN;

    Serial.println("[INIT] Smart Access Control ready.");
    Serial.printf("[INIT] PIN from NVS: %s | Failed: %d/%d\n",
                  "****", security.getFailedAttempts(), config_get_max_attempts());
    Serial.println("===========================================");
}

void loop()
{
    // Handle LVGL timers
    lv_timer_handler();

    // Screen transition state machine
    if (currentState == STATE_GRANTED && (millis() - stateTimer) > config_get_grant_duration_ms())
    {
        // Re-lock door and return to login screen after configured duration
        hw_door_lock();
        Serial.println("[STATE] Grant timeout — relocking door.");

        lv_obj_t *loginScreen = login_gen_create();
        lv_screen_load_anim(loginScreen, LV_SCREEN_LOAD_ANIM_FADE_IN, 300, 0, true);
        initialize_pin_ui();
        currentState = STATE_LOGIN;
    }
    else if (currentState == STATE_LOCKED && (millis() - stateTimer) > config_get_lockout_duration_ms())
    {
        // Reset security state and return to login after lockout duration
        security.reset();
        Serial.println("[STATE] Lockout timeout — resetting and returning to login.");

        lv_obj_t *loginScreen = login_gen_create();
        lv_screen_load_anim(loginScreen, LV_SCREEN_LOAD_ANIM_FADE_IN, 300, 0, true);
        initialize_pin_ui();
        currentState = STATE_LOGIN;
    }

    delay(5);
}

