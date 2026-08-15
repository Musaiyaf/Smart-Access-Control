// Minimal placeholder login (PIN entry) screen.
//
// This project expects a SquareLine Studio-exported login_gen.h/.c that was
// never committed to the repository, so main.cpp had nothing to build
// against. This hand-written stand-in provides the same interface
// (login_gen_create()) and wires up the same event callbacks
// (pin_key_event / clear_pin_event / submit_pin_event) that main.cpp
// already looks for, so the firmware builds and the PIN flow works.
// Replace with the real SquareLine export whenever it's available.
//
// LVGL v8 has no named-object lookup (that's a v9-only feature), so the
// pin/message labels are exposed via getters instead of a name-based search.

#include "login_gen.h"

extern "C" void pin_key_event(lv_event_t *event);
extern "C" void clear_pin_event(lv_event_t *event);
extern "C" void submit_pin_event(lv_event_t *event);

static const char *DIGIT_LABELS[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

static lv_obj_t *s_pin_label = nullptr;
static lv_obj_t *s_message_label = nullptr;

static lv_obj_t *make_key(lv_obj_t *parent, const char *text, lv_event_cb_t cb, void *user_data,
                           int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

lv_obj_t *login_gen_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x111827), 0);

    lv_obj_t *message_label = lv_label_create(screen);
    lv_obj_align(message_label, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_color(message_label, lv_color_hex(0x9CA3AF), 0);
    lv_label_set_text(message_label, "ENTER YOUR PIN");

    lv_obj_t *pin_label = lv_label_create(screen);
    lv_obj_align(pin_label, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_text_color(pin_label, lv_color_white(), 0);
    lv_label_set_text(pin_label, "");

    s_message_label = message_label;
    s_pin_label = pin_label;

    // 3x4 numeric keypad: 1-9, then CLR / 0 / OK
    const int32_t key_w = 72, key_h = 38, gap = 8;
    const int32_t grid_w = 3 * key_w + 2 * gap;
    const int32_t grid_x = (320 - grid_w) / 2;
    const int32_t grid_y = 60;

    const int layout[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, -1, 0, -2}; // -1 = CLR, -2 = OK

    for (int i = 0; i < 12; i++)
    {
        int row = i / 3;
        int col = i % 3;
        int32_t x = grid_x + col * (key_w + gap);
        int32_t y = grid_y + row * (key_h + gap);
        int v = layout[i];

        if (v >= 0)
        {
            make_key(screen, DIGIT_LABELS[v], pin_key_event, (void *)DIGIT_LABELS[v], x, y, key_w, key_h);
        }
        else if (v == -1)
        {
            make_key(screen, "CLR", clear_pin_event, NULL, x, y, key_w, key_h);
        }
        else
        {
            make_key(screen, "OK", submit_pin_event, NULL, x, y, key_w, key_h);
        }
    }

    return screen;
}

lv_obj_t *login_gen_get_pin_label(void)
{
    return s_pin_label;
}

lv_obj_t *login_gen_get_message_label(void)
{
    return s_message_label;
}
