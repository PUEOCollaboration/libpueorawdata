#ifndef _PUEO_RAWIO_VERSIONS_H
#define _PUEO_RAWIO_VERSIONS_H
/** old versions of types will go here */

#include "pueo/rawio.h"
#include "pueo/rawdata.h"
#include "pueo/pueo.h"

// make a prototype for pueo_X_size (ver)

#define X_SIZE(TYPE, STRUCT) \
int pueo_##STRUCT##_size(int ver);



PUEO_IO_DISPATCH_TABLE(X_SIZE)


#define CAT(a, ...) CAT_IMPL(a, __VA_ARGS__)
#define CAT_IMPL(a,...) a ## __VA_ARGS__


// These two macros just define pueo_X_vN_t as pueo_X_t, where N is the newest version

#define MAKE_CURRENT_VER_HELPER(STRUCT, VER) \
typedef pueo_##STRUCT##_t CAT(CAT(pueo_##STRUCT##_v,VER),_t);

#define X_CURRENT_VER(TYPE, STRUCT) \
  MAKE_CURRENT_VER_HELPER(STRUCT,TYPE##_VER)

PUEO_IO_DISPATCH_TABLE(X_CURRENT_VER)


// Here, we explicitly define any old versions (i.e. by copying the previous definition along with its version
//  (currently there are none!)





#endif















