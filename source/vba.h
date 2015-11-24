#ifndef __VBA_H__
#define __VBA_H__
#ifdef __cplusplus
extern "C" {
#endif


#include "gba.h"

/**
 * Logs a message message to VBA console (or GDB console if using GDB).
 */
IWRAM_CODE void vbalog(const char* msg);

#ifdef __cplusplus
}
#endif
#endif
