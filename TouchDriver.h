#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Select the touch controller type based on your hardware
// #define TOUCH_XPT2046        // SPI resistive touch
#define TOUCH_CST816S // I2C capacitive touch (your board)

    /**
     * @brief Initialize the touch controller.
     * - CST816S: I2C capacitive touch (SDA=GPIO4, SCL=GPIO5 by default)
     * - XPT2046: SPI resistive touch
     */
    void touch_init(void);

    /**
     * @brief LVGL v9 input device read callback.
     * Called by LVGL to poll touch state.
     *
     * @param indev  Pointer to the LVGL input device descriptor.
     * @param data   Pointer to output data structure to fill.
     */
    void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

    /**
     * @brief Check if touch is currently pressed.
     * @return true if touch detected.
     */
    bool touch_is_pressed(void);

    /**
     * @brief Get the last known X coordinate.
     */
    uint16_t touch_get_x(void);

    /**
     * @brief Get the last known Y coordinate.
     */
    uint16_t touch_get_y(void);

#ifdef __cplusplus
}
#endif

#endif // TOUCH_DRIVER_H
