#include "rawio_versions.h" 
#include <assert.h> 

// This file is truly horrifiying. 
// Basically, this generates a size table based on the latest version. 

// keep adding more size tables if needed 
#define SIZE_TABLE_0(STRUCT) \
static const int pueo_##STRUCT##_size_table[]  = { sizeof(pueo_##STRUCT##_v0_t)  

#define SIZE_TABLE_1(STRUCT) \
  SIZE_TABLE_0(STRUCT), sizeof(pueo_##STRUCT##_v1_t)

#define SIZE_TABLE_2(STRUCT) \
  SIZE_TABLE_1(STRUCT), sizeof(pueo_##STRUCT##_v2_t)

#define SIZE_TABLE_3(STRUCT) \
  SIZE_TABLE_2(STRUCT), sizeof(pueo_##STRUCT##_v3_t)

#define SIZE_TABLE_4(STRUCT) \
  SIZE_TABLE_3(STRUCT), sizeof(pueo_##STRUCT##_v4_t)

#define SIZE_TABLE_5(STRUCT) \
  SIZE_TABLE_4(STRUCT), sizeof(pueo_##STRUCT##_v5_t)

#define SIZE_TABLE_6(STRUCT) \
  SIZE_TABLE_5(STRUCT), sizeof(pueo_##STRUCT##_v6_t)

#define SIZE_TABLE_7(STRUCT) \
  SIZE_TABLE_6(STRUCT), sizeof(pueo_##STRUCT##_v7_t)

#define SIZE_TABLE_8(STRUCT) \
  SIZE_TABLE_7(STRUCT), sizeof(pueo_##STRUCT##_v8_t)

#define SIZE_TABLE_9(STRUCT) \
  SIZE_TABLE_8(STRUCT), sizeof(pueo_##STRUCT##_v9_t)

#define SIZE_TABLE_10(STRUCT) \
  SIZE_TABLE_9(STRUCT), sizeof(pueo_##STRUCT##_v10_t)

#define SIZE_TABLE_HELPER(STRUCT,VER) \
  CAT(SIZE_TABLE_,VER(STRUCT))};

#define X_SIZE_TABLE(TYPE,STRUCT) \
SIZE_TABLE_HELPER(STRUCT, TYPE##_VER)

PUEO_IO_DISPATCH_TABLE(X_SIZE_TABLE)

#define X_SIZE_IMPL(TYPE, STRUCT) \
int pueo_##STRUCT##_size(int ver) \
{ \
  if (ver > TYPE##_VER || ver < 0) {return -1;} \
  static_assert(sizeof(pueo_##STRUCT##_size_table) == 4*(1+TYPE##_VER), "SIZE TABLE WRONG"); \
  return pueo_##STRUCT##_size_table[ver]; \
}

PUEO_IO_DISPATCH_TABLE(X_SIZE_IMPL)
