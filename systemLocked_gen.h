#ifndef SYSTEMLOCKED_GEN_H
#define SYSTEMLOCKED_GEN_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Create the system-locked screen.
     * @return Pointer to the created screen object.
     */
    lv_obj_t *systemLocked_gen_create(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEMLOCKED_GEN_H
