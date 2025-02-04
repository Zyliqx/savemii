#include "sd_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __sd_fs_dirreset(struct _reent *r,
                     DIR_ITER *dirState) {
    if (!dirState) {
        r->_errno = EINVAL;
        return -1;
    }

    __sd_fs_dir_t *dir = (__sd_fs_dir_t *) (dirState->dirStruct);
    FRESULT fr = f_rewinddir(&dir->dp);
    if (fr != FR_OK) {
        r->_errno = __sd_fs_translate_error(fr);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif