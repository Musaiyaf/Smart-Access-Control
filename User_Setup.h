// Minimal User_Setup.h for TFT_eSPI on ESP32-S3 with ILI9341
// SPI interface, no fonts loaded, no touch support (using our own TouchDriver)

#define ILI9341_DRIVER

// SPI pins
#define TFT_CS   10
#define TFT_DC    6
#define TFT_RST   7
#define TFT_MOSI  11
#define TFT_MISO  13
#define TFT_SCLK  12

// Backlight (fixed on VIEWE ESP32-S3 2.8" module)
#define TFT_BL    1

// SPI frequency
#define SPI_FREQUENCY  40000000

// Minimal fonts — GLCD is required for TFT_eSPI .print() splash text
#define LOAD_GLCD  1
#define LOAD_FONT2 0
#define LOAD_FONT4 0
#define LOAD_FONT6 0
#define LOAD_FONT7 0
#define LOAD_FONT8 0
#define LOAD_FONT8N 0
#define LOAD_GFXFF 0

// Smooth font off
#define SMOOTH_FONT 0

// No touch support (using our own touch driver)
#define TOUCH_CS -1

