#include "TouchDriver.h"
#include <Arduino.h>
#include <Wire.h>

// =========================================================
// CST816S Capacitive Touch Driver (I2C)
// =========================================================

#define CST816S_ADDR 0x15
#define CST816S_REG_GESTURE 0x01
#define CST816S_REG_TOUCH 0x03 // Touch points
#define CST816S_REG_X_H 0x03   // Touch status + X high nibble
#define CST816S_REG_X_L 0x04   // X low byte
#define CST816S_REG_Y_H 0x05   // Y high nibble
#define CST816S_REG_Y_L 0x06   // Y low byte

// I2C retry configuration for CST816S (first read often fails)
#define CST816S_I2C_RETRY_COUNT 3
#define CST816S_I2C_RETRY_DELAY_MS 5

#ifndef CST816S_SDA
#define CST816S_SDA 4
#endif
#ifndef CST816S_SCL
#define CST816S_SCL 5
#endif
#ifndef CST816S_RST
#define CST816S_RST 3 // Reset pin (fixed on VIEWE module)
#endif
#ifndef CST816S_INT
#define CST816S_INT 0 // Interrupt pin (fixed on VIEWE module; polled, not wired as an ISR)
#endif

static uint16_t last_x = 0;
static uint16_t last_y = 0;
static bool touch_pressed = false;

// =========================================================
// CST816S Low-Level I2C
// =========================================================

static bool cst816s_write_reg(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(CST816S_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

static bool cst816s_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    // Retry loop: CST816S often NAKs the first I2C transaction after power-up
    for (uint8_t retry = 0; retry < CST816S_I2C_RETRY_COUNT; retry++)
    {
        Wire.beginTransmission(CST816S_ADDR);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0)
        {
            delay(CST816S_I2C_RETRY_DELAY_MS);
            continue;
        }
        Wire.requestFrom((int)CST816S_ADDR, (int)len);
        bool all_read = true;
        for (uint8_t i = 0; i < len; i++)
        {
            if (Wire.available())
            {
                buf[i] = Wire.read();
            }
            else
            {
                all_read = false;
                break;
            }
        }
        if (all_read)
        {
            return true;
        }
        delay(CST816S_I2C_RETRY_DELAY_MS);
    }
    return false;
}

// =========================================================
// CST816S Initialization
// =========================================================

static void cst816s_init(void)
{
    Serial.println("[TOUCH] Initializing CST816S capacitive touch (I2C)...");

    // Optional reset
    if (CST816S_RST >= 0)
    {
        pinMode(CST816S_RST, OUTPUT);
        digitalWrite(CST816S_RST, LOW);
        delay(10);
        digitalWrite(CST816S_RST, HIGH);
        delay(50);
    }

    Wire.begin(CST816S_SDA, CST816S_SCL);
    Wire.setClock(400000); // Fast mode 400kHz for responsive touch

    // Read chip ID to verify
    uint8_t chip_id = 0;
    if (cst816s_read_regs(0xA7, &chip_id, 1))
    {
        Serial.printf("[TOUCH] CST816S chip ID: 0x%02X\n", chip_id);
        if (chip_id != 0xB4 && chip_id != 0xB5 && chip_id != 0xB6)
        {
            Serial.printf("[TOUCH] WARNING: Unexpected chip ID (expected 0xB4/B5/B6)\n");
        }
    }
    else
    {
        Serial.println("[TOUCH] WARNING: Could not read chip ID");
    }

    // Configure CST816S for continuous reporting
    cst816s_write_reg(0xFE, 0x00); // Disable sleep
    cst816s_write_reg(0xED, 0x01); // Enable auto-report

    Serial.println("[TOUCH] CST816S initialized.");
}

// =========================================================
// CST816S Read Touch Point
// =========================================================

static bool cst816s_read_touch(uint16_t *x, uint16_t *y)
{
    uint8_t buf[6] = {0};

    if (!cst816s_read_regs(CST816S_REG_TOUCH, buf, 6))
    {
        return false;
    }

    // Byte 0: touch count (0 = no touch)
    if (buf[0] == 0)
    {
        return false;
    }

    // Bytes 1-2: X coordinate (big-endian 12-bit)
    // Bytes 3-4: Y coordinate (big-endian 12-bit)
    *x = ((uint16_t)(buf[1] & 0x0F) << 8) | buf[2];
    *y = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4];

    return true;
}

// =========================================================
// Public API
// =========================================================

void touch_init(void)
{
    cst816s_init();
}

bool touch_is_pressed(void)
{
    return touch_pressed;
}

uint16_t touch_get_x(void)
{
    return last_x;
}

uint16_t touch_get_y(void)
{
    return last_y;
}

void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    uint16_t raw_x, raw_y;

    if (cst816s_read_touch(&raw_x, &raw_y))
    {
        // Normalize to 320x240 display
        // CST816S returns 0-240 for X in portrait, map to 0-319 landscape
        // These values depend on the specific panel orientation
        if (raw_x < 320 && raw_y < 240)
        {
            last_x = raw_x;
            last_y = raw_y;
        }
        else if (raw_y < 320 && raw_x < 240)
        {
            // Swapped axes — swap them back
            last_x = raw_y;
            last_y = raw_x;
        }

        touch_pressed = true;
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        touch_pressed = false;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
