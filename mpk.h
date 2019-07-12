/*MPK - Copyright 2019 Joshua Barrett
  This software is BSD Licensed.*/
#ifndef MPK_H
#define MPK_H
#include <stdint.h>

typedef struct {
    size_t size;
    uint8_t *msg;
} mpk_msg_t;

enum{
    MPK_STR,
    MPK_BIN,
    MPK_EXT
};

typedef struct {
    size_t off; //offset to the start of the DATA.
    uint32_t size;
    uint8_t *data; //Pointer to the data. For external use.
    int8_t type[2]; //type[1] is only valid for exts.
} mpk_blob_t;

enum{
    MPK_ARR,
    MPK_MAP
};

typedef struct {
    mpk_msg_t *msg;
    size_t off; 
    uint32_t elems;
    int8_t type;
} mpk_collection_t;


//this only defines types that are composites. The rest are defined by their
//value as used in the actual encoding, which is in the enum below.
enum {
    MPK_BLOB,
    MPK_COL,
    MPK_BOOL
};

enum {
    MPK_UF   = 0x00,
    MPK_MF   = 0x80,
    MPK_AF   = 0x90,
    MPK_SF   = 0xa0,
    MPK_NIL  = 0xc0,
    MPK_F    = 0xc2,
    MPK_T    = 0xc3,
    MPK_B8   = 0xc4,
    MPK_B16  = 0xc5,
    MPK_B32  = 0xc6,
    MPK_X8   = 0xc7,
    MPK_X16  = 0xc8,
    MPK_X32  = 0xc9,
    MPK_F32  = 0xca,
    MPK_F64  = 0xcb,
    MPK_U8   = 0xcc,
    MPK_U16  = 0xcd,
    MPK_U32  = 0xce,
    MPK_U64  = 0xcf,
    MPK_I8   = 0xd0,
    MPK_I16  = 0xd1,
    MPK_I32  = 0xd2,
    MPK_I64  = 0xd3,
    MPK_XF1  = 0xd4,
    MPK_XF2  = 0xd5,
    MPK_XF4  = 0xd6,
    MPK_XF8  = 0xd7,
    MPK_XF16 = 0xd8,
    MPK_S8   = 0xd9,
    MPK_S16  = 0xda,
    MPK_S32  = 0xdb,
    MPK_A16  = 0xdc,
    MPK_A32  = 0xdd,
    MPK_M16  = 0xde,
    MPK_M32  = 0xdf,
    MPK_IF   = 0xe0
};

typedef struct {
    union{
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	int8_t i8;
	int16_t i16;
	int32_t i32;
	int64_t i64;
	float f32;
	double f64;
	int8_t bool;
	int8_t nil;
	mpk_collection_t col;
	mpk_blob_t blob;
    } value;
    size_t rawhdr; //offset to header.
    uint8_t type;
} mpk_value_t;


#ifdef __cplusplus
extern "C" {
#endif
    int MPK_ElemAt(mpk_msg_t *a, uint8_t off, mpk_value_t* out);
    int MPK_IndexArr(mpk_collection_t *arr, uint32_t idx, mpk_value_t *out);
    int MPK_FindInMap(mpk_collection_t *map, mpk_value_t *k, mpk_value_t *v);
    uint8_t *MPK_ColToRawPtr(mpk_value_t *val);
    size_t MPK_ColSize(mpk_value_t *val);
#ifdef __cplusplus
}
#endif


#endif //EOF
