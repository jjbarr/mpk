/*MPK - Copyright 2019 Joshua Barrett.
  This software is BSD licensed.*/
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include "mpk.h"

#if defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#elif defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN)
#else
#error "Either BIG_ENDIAN or LITTLE_ENDIAN must be defined, as per your arch."
#endif

///INDEX Macro.
//Returns a reference to an index. Or null if the index is >= m.
//is NOT guaranteed to eval args only once. Use with caution.
#define INDEX(a, i, m) ((i < m)?&(a[i]):NULL)

///MPK_UnsafeBytesToNum
//swaps bytes in byte array as necessary,
//and returns a uint64_t containing bytes from source v.
//v isn't bounds checked, hence the "unsafe".
//the resultant uint64_t is not necessarily a correct number until
//it is reinterpreted using a union as the correct type.

uint64_t MPK_UnsafeBytesToNum(uint8_t *v, size_t bytes){
    union {uint64_t n; uint8_t b[8];} rv;
    rv.n = 0;
    memcpy(&rv.b, v, bytes);
#ifdef LITTLE_ENDIAN
    for(int i = 0; i < bytes / 2; i++){
	uint8_t tmp = rv.b[i];
	rv.b[i] = rv.b[bytes - 1 - i];
	rv.b[bytes - 1 - i] = tmp;
    }
#endif
    return rv.n;
}

///Type identification.



//Type is usually the first byte of the header.
//However, there are a few types that encode length data
//within the byte.
//The "magic numbers" mask off data bits to test for these.
//(but if you're an experienced C programmer you probably already knew that.)

//INclusive range test.
#define IN_RANGEI(s, n, e) ((s) <= (n) && (n) <= (e))

//FIXes.
//doesn't cover fixexts. They're special.
#define MPK_ISFIX(t) (!((t) & 0x80) || !((t) & 0x40) || ((t) & 0xe0) == 0xe0)
#define MPK_ISUF(t) (((t) & 0x80) == MPK_UF)
#define MPK_ISMF(t) (((t) & 0xf0) == MPK_MF)
#define MPK_ISAF(t) (((t) & 0xf0) == MPK_AF)
#define MPK_ISSF(t) (((t) & 0xe0) == MPK_SF)
#define MPK_ISIF(t) (((t) & 0xe0) == MPK_IF)

//fixexts are covered here
#define MPK_ISXF(t)(IN_RANGEI(MPK_XF1, t, MPK_XF16))

//Collections
#define MPK_ISARR(t) (MPK_ISAF(t) || IN_RANGEI(MPK_A16, t, MPK_A32))
#define MPK_ISMAP(t) (MPK_ISMF(t) || IN_RANGEI(MPK_M16, t, MPK_M32))
#define MPK_ISCOL(t) (MPK_ISARR(t) || MPK_ISMAP(t))

//blobs
#define MPK_ISEXT(t) (MPK_ISXF(t) || IN_RANGEI(MPK_X8, t, MPK_X32))
#define MPK_ISSTR(t) (MPK_ISSF(t) || IN_RANGEI(MPK_S8, t, MPK_S32))
#define MPK_ISBIN(t) (IN_RANGEI(MPK_B8, t, MPK_B32))
#define MPK_ISBLOB(t) (MPK_ISEXT(t)||MPK_ISSTR(t)||MPK_ISBIN(t))

//numerics
#define MPK_ISNUM(t) \
    (MPK_ISUF(t)||MPK_ISIF(t)||IN_RANGEI(MPK_F32, t, MPK_I64))

//Const
#define MPK_ISNIL(t) ((t) == MPK_NIL)
#define MPK_ISBOOL(t) ((t) == MPK_T || (t) == MPK_F)

//returns the size of static-sized types (including header!)
//returns zero for dynamic-sized types.
int MPK_STSiz(uint8_t t){
    if(MPK_ISUF(t) || MPK_ISIF(t) || MPK_ISBOOL(t) || MPK_ISNIL(t)) return 1;
    switch(t){
    case MPK_U8: case MPK_I8:
	return 2;
    case MPK_U16: case MPK_I16:
	return 3;
    case MPK_U32: case MPK_I32: case MPK_F32:
	return 5;
    case MPK_U64: case MPK_I64: case MPK_F64:
	return 9;
    }
    return 0;
}

//The MPKNUM macro generates MPK_Num<type> functions
//(except MPK_NumUF and MPK_NumIF). The function prototype at the end
//is a hack so that you can put a semicolon after the macro as you'd expect.

//the MPK_Num family of functions take a message as an mpk_msg_t,
//the offset the number starts at (INcluding the header),
//and an error pointer. error will be set to 1 if the requested
//number overflows the end of the message.

#define MPKNUM(NAME, CTYPE, bytes)				\
    CTYPE MPK_Num ## NAME (mpk_msg_t *a, size_t off, int *err){	\
	uint8_t *num;						\
	if((num = INDEX(a->msg, off + 1, a->size))		\
	   && INDEX(a->msg, off + bytes, a->size)){		\
	    union {uint64_t in; CTYPE out;} rv;			\
	    rv.in = MPK_UnsafeBytesToNum(num, bytes);		\
	    return rv.out;					\
	}else{							\
	    *err = 1;						\
	    return 0;						\
	}							\
    }								\
    CTYPE MPK_Num ## NAME (mpk_msg_t *a, size_t off, int *err)

MPKNUM(F32, float, 4);
MPKNUM(F64, float, 8);
MPKNUM(U8, uint8_t, 1);
MPKNUM(U16, uint16_t, 2);
MPKNUM(U32, uint32_t, 4);
MPKNUM(U64, uint64_t, 8);
MPKNUM(I8, int8_t, 1);
MPKNUM(I16, int16_t, 2);
MPKNUM(I32, int32_t, 4);
MPKNUM(I64, int64_t, 8);

uint8_t MPK_NumUF(mpk_msg_t *a, size_t off, int *err){
    uint8_t *num;
    if((num = INDEX(a->msg, off, a->size))){
	return *num;
    }else{
	*err = 1;
	return 0;
    }
}

int8_t MPK_NumIF(mpk_msg_t *a, size_t off, int *err){
    uint8_t *num;
    if((num = INDEX(a->msg, off, a->size))){
	return (int8_t)*num;
    }else{
	*err = 1;
	return 0;
    }
}
#undef MPKNUM

//TODO: Find a way to macroize the collections code. It's all very similar.
//as in, I literally copy-pasted it.

//Unlike the MPK_Num family, MPK_Blob can signal an error
//by setting the off field to 0 (0 is never a valid data offset).

mpk_blob_t MPK_Blob(mpk_msg_t *a, size_t off){
    uint8_t* p;
    int ibytes = 0;
    mpk_blob_t rv;
    rv.size = 0;
    rv.data = NULL;
    rv.type[1] = 0;
    rv.off = 0;
    if((p = INDEX(a->msg, off, a->size))){
	if(MPK_ISSTR(*p)) rv.type[0] = MPK_STR;
	else if(MPK_ISBIN(*p)) rv.type[0] = MPK_BIN;
	else if(MPK_ISEXT(*p)) rv.type[0] = MPK_EXT;
	else return rv;
	if(rv.type[0] == MPK_STR && MPK_ISSF(*p)){
	    rv.size = *p & 0x1f;
	}else if(rv.type[0] == MPK_EXT && MPK_ISXF(*p)){
	    rv.size = 1 << (*p - MPK_XF1);
	}else{
	    int err = 0;
	    switch(*p){
	    case MPK_B8:
	    case MPK_S8:
	    case MPK_X8:
		ibytes = 1;
		rv.size = MPK_NumU8(a, off, &err);
		break;
	    case MPK_B16:
	    case MPK_S16:
	    case MPK_X16:
		ibytes = 2;
		rv.size = MPK_NumU16(a, off, &err);
		break;
	    case MPK_B32:
	    case MPK_S32:
	    case MPK_X32:
		ibytes = 4;
		rv.size = MPK_NumU32(a, off, &err);
		break;
	    default:
		return rv; //Can't Happen (TM)
	    }
	    if(err) return rv;
	}
    }else{
	return rv;
    }

    if(rv.type[0] == MPK_EXT){
	if((p = INDEX(a->msg, off + ibytes + 1, a->size))) //ext's typespec...
	    rv.type[1] = (int8_t)*p;
	else return rv;
    }

    //eod == End Of Data. There's an extra +1 for ext to account for typespec.
    size_t eod = off + ibytes + rv.size + ((rv.type[0] == MPK_EXT)?1:0);
    if(INDEX(a->msg, eod, a->size)){
	//doff = data offset. See above for explanation of the +1.
	size_t doff = off + ibytes + ((rv.type[0] == MPK_EXT)?1:0) + 1;
	rv.data = (a->msg + doff);
	rv.off = doff;
    }
    return rv;
}

mpk_collection_t MPK_Col(mpk_msg_t *a, size_t off){
    uint8_t* p;
    int ibytes = 0;
    mpk_collection_t rv;
    rv.elems = 0;
    rv.msg = a;
    rv.off = 0; //data offset will never be zero.
    if((p = INDEX(a->msg, off, a->size))){
	if(MPK_ISARR(*p)) rv.type = MPK_ARR;
	else if(MPK_ISMAP(*p)) rv.type = MPK_MAP;
	else return rv;

	if (MPK_ISFIX(*p)) {
	    rv.elems = *p & 0x0f;
	}else{
	    int numerr = 0;
	    switch(*p){	
	    case MPK_M16:
	    case MPK_A16:
		ibytes = 2;
		rv.elems = MPK_NumU16(a, off, &numerr);
		break;
	    case MPK_M32:
	    case MPK_A32:
		ibytes = 4;
		rv.elems = MPK_NumU32(a, off, &numerr);
		break;
	    default:
		return rv;
	    }
	    if(numerr) return rv;
	}
    }else{
	return rv;
    }
    size_t data_off = off + 1 + ibytes;
    if (INDEX(a->msg, data_off, a->size)) rv.off = data_off;
    return rv;
}

//this defines handlers for the different numeric types.
//they're all the same.
#define NUMERIC(T,slot) case MPK_ ## T: out->type = MPK_ ## T;	\
    out->value.slot = MPK_Num ## T (a, off, &err);		\
    break

int MPK_ElemAt(mpk_msg_t *a, uint8_t off, mpk_value_t* out){
    uint8_t *p;
    if((p = INDEX(a->msg, off, a->size))){
	out->rawhdr = off;
	if(MPK_ISBLOB(*p)){
	    out->type = MPK_BLOB;
	    out->value.blob = MPK_Blob(a, off);
	    if(out->value.blob.off == 0) return 0;
	}else if(MPK_ISCOL(*p)){
	    out->type = MPK_COL;
	    out->value.col = MPK_Col(a, off);
	    if(out->value.col.off == 0) return 0;
	}else if(MPK_ISBOOL(*p)){
	    out->type = MPK_BOOL;
	    out->value.bool = (*p==MPK_T);
	}else if(MPK_ISNIL(*p)){
	    out->type = MPK_NIL;
	    out->value.nil = 0;
	}else if(MPK_ISNUM(*p)){
	    int err = 0;
	    if(MPK_ISUF(*p)){
		out->type = MPK_U8;
		out->value.u8 = MPK_NumUF(a, off, &err);
	    }else if(MPK_ISIF(*p)){
		out->type = MPK_I8;
		out->value.i8 = MPK_NumIF(a, off, &err);
	    }else{
		switch(*p){
		    NUMERIC(U8, u8);
		    NUMERIC(U16, u16);
		    NUMERIC(U32, u32);
		    NUMERIC(U64, u64);
		    NUMERIC(I8, i8);
		    NUMERIC(I16, i16);
		    NUMERIC(I32, i32);
		    NUMERIC(I64, i64);
		    NUMERIC(F32, f32);
		    NUMERIC(F64, f64);
		default: //Can't Happen (TM)
		    return 0;
		} 
	    }
	    if(err) return 0;
	}else return 0;
	return 1;
    }else return 0;
}
	    
#undef NUMERIC
    // destructively advances *start n elems forward through a.
    // returns zero on failure.
int MPK_FwdNElems(mpk_msg_t *a, mpk_value_t *start, size_t n){
    size_t off = start->rawhdr;
    if(off >= a->size) return 0;
    uint8_t* p;
    for(;n!=0 && off < a->size ;n--){
	if((p = INDEX(a->msg, off, a->size))){
	    if(MPK_STSiz(*p)){
		off += MPK_STSiz(*p);
	    }else if(start->type == MPK_BLOB){
		off = start->value.blob.off + start->value.blob.size;
	    }else if(start->type == MPK_COL){
		off = start->value.col.off;
		n += start->value.col.elems
		    * ((start->value.col.type==MPK_MAP)?2:1); //MAP: 2x elems
		}else return 1;
	    if(!MPK_ElemAt(a, off, start)) return 0;
	    off = start->rawhdr;
	}
    }
    if(off >= a->size) return 0;
    return 1;
}

//retrieves elem at index N of MPK array, placing it in out.
//array is zero-indexed.
//returns zero on failure.
int MPK_IndexArr(mpk_collection_t *arr, uint32_t idx, mpk_value_t *out){
    if(arr->type != MPK_ARR) return 0;
    if(idx >= arr->elems) return 0;
    if(!MPK_ElemAt(arr->msg, arr->off, out)) return 0;
    return MPK_FwdNElems(arr->msg, out, idx);
}

//retrives value into v that corresponds to key k in map.
//keys are deep-compared. However, map or array keys are not supported.
//this is a limitation, and may be fixed in the future.
//returns zero on failure.
int MPK_FindInMap(mpk_collection_t *map, mpk_value_t *k, mpk_value_t *v){
    if(map->type != MPK_MAP) return 0;
    if(k->type == MPK_COL) return 0;
    if(!MPK_ElemAt(map->msg, map->off, v)) return 0;
    for(int i=0; i < map->elems; i++){
	if(k->type == v->type){
	    int eq = 0;
	    if(k->type == MPK_BLOB){
		eq = (k->value.blob.size == v->value.blob.size
		      && !memcmp(k->value.blob.type, v->value.blob.type, 2)
		      && !memcmp(k->value.blob.data, v->value.blob.data,
				 k->value.blob.size));
	    }else{
		
		/*MACRO*/
#define EQUALAS(T, slot)						\
		case T: eq = k->value.slot == v->value.slot; break
		/*ENDMACRO*/
		
		switch(k->type){
		    EQUALAS(MPK_BOOL, bool);
		    EQUALAS(MPK_NIL, nil);
		    EQUALAS(MPK_F32, f32);
		    EQUALAS(MPK_F64, f64);
		    EQUALAS(MPK_U8, u8);
		    EQUALAS(MPK_U16, u16);
		    EQUALAS(MPK_U32, u32);
		    EQUALAS(MPK_U64, u64);
		    EQUALAS(MPK_I8, i8);
		    EQUALAS(MPK_I16, i16);
		    EQUALAS(MPK_I32, i32);
		    EQUALAS(MPK_I64, i64);
		}
#undef EQUALAS
	    }
	    if(eq) return MPK_FwdNElems(map->msg, v, 1);
	    if(!MPK_FwdNElems(map->msg, v, 2)) break;
	}
    }
    return 0;
}


//for use when copying collections.
uint8_t *MPK_ColToRawPtr(mpk_value_t *val){
    if(val->type != MPK_COL) return NULL;
    mpk_msg_t *m = val->value.col.msg;
    return INDEX(m->msg, val->rawhdr, m->size); 
}

//returns invalid size of zero as error, as header is included in size.
size_t MPK_ColSize(mpk_value_t *val){
    if(val->type != MPK_COL) return 0;
    uint32_t ielems = val->value.col.elems;
    if(val->value.col.type == MPK_MAP) ielems *= 2;
    mpk_value_t endel;
    if(!MPK_ElemAt(val->value.col.msg, val->value.col.off, &endel))
	return 0;
    if(!MPK_FwdNElems(val->value.col.msg, &endel, ielems))
	return val->value.col.msg->size - val->rawhdr;
    return endel.rawhdr - val->rawhdr;
}
