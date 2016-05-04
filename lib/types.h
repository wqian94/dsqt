/**
Types definitions
*/

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "rlu.h"

typedef float float32_t;
typedef double float64_t;
typedef long double float128_t;

/*typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;*/

#define safe __attribute__((transaction_safe))

#endif
