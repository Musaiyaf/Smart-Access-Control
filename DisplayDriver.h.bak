#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize the ILI9341 display via TFT_eSPI.
     * Configures SPI, backlight PWM, rotation, and initial brightness.
     */
    void display_init(void);

    /**
     * @brief LVGL v9 display flush callback.
     * Called by LVGL to send a rendered buffer region to the TFT.
     *
     * @param disp  Pointer to the LVGL display descriptor.
     * @param area  Rectangle area to flush.
     * @param px_map Pointer to pixel data (RGB565).
     */
    void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

    /**
     * @brief Set backlight brightness (PWM).
     *
     * @param brightness 0–255 value (0 = off, 255 = full).
     */
    void display_set_backlight(uint8_t brightness);

    /**
     * @brief Get the internal TFT_eSPI instance for direct calls if needed.
     */
    TFT_eSPI *display_get_tft(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_DRIVER_H
