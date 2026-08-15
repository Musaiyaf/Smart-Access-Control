// Minimal placeholder system-locked screen.
// See login_gen.cpp for why this hand-written stand-in exists.

#include "systemLocked_gen.h"

lv_obj_t *systemLocked_gen_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x7F1D1D), 0);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "SYSTEM LOCKED");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    return screen;
}
