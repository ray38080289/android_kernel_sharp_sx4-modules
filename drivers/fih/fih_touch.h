#ifndef __FIH_TOUCH_H
#define __FIH_TOUCH_H

struct fih_touch_cb {
    void (*touch_selftest)(void);
    int (*touch_selftest_result)(void);
    void (*touch_tpfwver_read)(char *);
    void (*touch_fwupgrade)(int);
    void (*touch_fwupgrade_read)(char *);
    void (*touch_vendor_read)(char *);
};

#endif /* __FIH_TOUCH_H */
