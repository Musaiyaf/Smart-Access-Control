#ifndef LOGIN_GEN_H
#define LOGIN_GEN_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Create the login (PIN entry) screen.
     * @return Pointer to the created screen object.
     */
    lv_obj_t *login_gen_create(void);

#ifdef __cplusplus
}
#endif

#endif // LOGIN_GEN_H
