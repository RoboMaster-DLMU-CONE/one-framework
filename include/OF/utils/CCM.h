#ifndef OF_CCM_H
#define OF_CCM_H

#include <zephyr/kernel.h>

#if DT_HAS_CHOSEN(zephyr_ccm)
#define OF_CCM_ATTR __ccm_noinit_section
#else
// 如果没有 CCM，定义为空（即使用默认的 RAM）
#define OF_CCM_ATTR
#endif

#endif //OF_CCM_H
