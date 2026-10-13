#ifndef STDINT_H
#define STDINT_H

/* 8-bit types */
typedef __INT8_TYPE__   int8_t;
typedef __UINT8_TYPE__  uint8_t;

/* 16-bit types */
typedef __INT16_TYPE__  int16_t;
typedef __UINT16_TYPE__ uint16_t;

/* 32-bit types */
typedef __INT32_TYPE__  int32_t;
typedef __UINT32_TYPE__ uint32_t;

/* 64-bit types */
typedef __INT64_TYPE__  int64_t;
typedef __UINT64_TYPE__ uint64_t;

/* Integers wide enough to hold a pointer */
typedef __INTPTR_TYPE__  intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;

#endif