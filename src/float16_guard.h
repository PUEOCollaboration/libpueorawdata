#ifndef _flt16_guard_h
#define _flt16_guard_h

#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
#define __STDC_WANT_IEC_60559_TYPES_EXT__
#endif
#include <float.h>
#ifndef FLT16_MAX
#error Compiling libpueorawdata needs Float16 support (i.e. GCC += 12)
#endif

#endif
