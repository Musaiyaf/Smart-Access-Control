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

// Minimal fonts — nothing is drawn via TFT_eSPI's print()/write() (LVGL
// owns all screen content), so no fonts need to be loaded here.
#define LOAD_GLCD  1
#define LOAD_FONT2 0
#define LOAD_FONT4 0
#define LOAD_FONT6 0
#define LOAD_FONT7 0
#define LOAD_FONT8 0
#define LOAD_FONT8N 0
#define LOAD_GFXFF 0

// Smooth font intentionally left undefined: TFT_eSPI gates its VLW
// smooth-font code with `#ifdef SMOOTH_FONT`, which is true as soon as
// the macro exists — even as `SMOOTH_FONT 0` — so defining it at all
// compiles in the smooth-font/write() code path we don't use.

// No touch support (using our own touch driver)
#define TOUCH_CS -1

