#include "DisplayDriver.h"
#include <Arduino.h>

// ------------------------------------------------------------------
// LEDC PWM channel for display backlight (separate from buzzer channel)
// ------------------------------------------------------------------
#define BL_LEDC_CHANNEL  1
#define BL_LEDC_RES      8   // 8-bit resolution (0-255)
#define BL_LEDC_FREQ     5000 // 5 kHz PWM frequency

// ------------------------------------------------------------------
// Pin definitions (defaults; can be overridden via build_flags)
// ------------------------------------------------------------------
#ifndef TFT_CS
#define TFT_CS 10
#endif
#ifndef TFT_DC
#define TFT_DC 9
#endif
#ifndef TFT_RST
#define TFT_RST 8
#endif
#ifndef TFT_MOSI
#define TFT_MOSI 11
#endif
#ifndef TFT_MISO
#define TFT_MISO 13
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 12
#endif
#ifndef TFT_BL
#define TFT_BL 38
#endif

// TFT_eSPI uses user_setup.h or build flags for pin configuration
static TFT_eSPI tft = TFT_eSPI(320, 240);

// ------------------------------------------------------------------
// Initialization
// ------------------------------------------------------------------
void display_init(void)
{
    Serial.println("[DISPLAY] Initializing ILI9341 via TFT_eSPI...");

    tft.begin();
    tft.setRotation(1); // Landscape orientation (320x240)
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 100);
    tft.print("Smart Access Control");

    // Configure backlight PWM on TFT_BL pin using ledc (reliable on ESP32-S3)
    pinMode(TFT_BL, OUTPUT);
    ledcSetup(BL_LEDC_CHANNEL, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(TFT_BL, BL_LEDC_CHANNEL);
    ledcWrite(BL_LEDC_CHANNEL, 192); // ~75% brightness

    Serial.println("[DISPLAY] ILI9341 initialized successfully.");
    Serial.printf("[DISPLAY] Resolution: %dx%d\n", tft.width(), tft.height());
}

// ------------------------------------------------------------------
// LVGL v9 Flush Callback
// ------------------------------------------------------------------
void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)disp;

    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    tft.setAddrWindow(
        area->x1,
        area->y1,
        area->x1 + w - 1,
        area->y1 + h - 1);

    // Push pixels in RGB565 format
    tft.pushColors((uint16_t *)px_map, w * h, true);

    // Notify LVGL that flushing is complete
    lv_display_flush_ready(disp);
}

// ------------------------------------------------------------------
// Backlight Control
// ------------------------------------------------------------------
void display_set_backlight(uint8_t brightness)
{
    ledcWrite(BL_LEDC_CHANNEL, brightness);
}

// ------------------------------------------------------------------
// TFT Accessor
// ------------------------------------------------------------------
TFT_eSPI *display_get_tft(void)
{
    return &tft;
}
