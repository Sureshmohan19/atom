// atom_types.c
// Implementation of the Atom type system.

#include "atom_types.h"
#include <stdint.h>

// --- Static DType Definitions ---

// Boolean type
static const Atom_DType g_atom_bool_dtype = {
    .type_id   = ATOM_BOOL,
    .type_char = ATOM_BOOL_CHAR,
    .kind      = 'b', 
    .byteorder = ATOM_BYTEORDER_NA,
    .elsize    = sizeof(uint8_t), // Often stored as a byte
    .alignment = _Alignof(uint8_t),
    .name      = "bool",
};

// Integer types
// Signed 8-bit integer
static const Atom_DType g_atom_int8_dtype = {
    .type_id    = ATOM_INT8,
    .type_char  = ATOM_INT8_CHAR,
    .kind       = 'i', 
    .byteorder  = ATOM_BYTEORDER_NA,
    .elsize     = sizeof(int8_t),
    .alignment  = _Alignof(int8_t),
    .name       = "int8",
};

// Unsigned 8-bit integer
static const Atom_DType g_atom_uint8_dtype = {
    .type_id   = ATOM_UINT8,
    .type_char = ATOM_UINT8_CHAR,
    .kind      = 'u', 
    .byteorder = ATOM_BYTEORDER_NA,
    .elsize    = sizeof(uint8_t),
    .alignment = _Alignof(uint8_t),
    .name      = "uint8",
};

// Signed 16-bit integer
static const Atom_DType g_atom_int16_dtype = {
    .type_id   = ATOM_INT16,
    .type_char = ATOM_INT16_CHAR,
    .kind      = 'i',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(int16_t),
    .alignment = _Alignof(int16_t),
    .name      = "int16",
};

// Unsigned 16-bit integer
static const Atom_DType g_atom_uint16_dtype = {
    .type_id   = ATOM_UINT16,
    .type_char = ATOM_UINT16_CHAR,
    .kind      = 'u',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(uint16_t),
    .alignment = _Alignof(uint16_t),
    .name      = "uint16",
};

// Signed 32-bit integer
static const Atom_DType g_atom_int32_dtype = {
    .type_id   = ATOM_INT32,
    .type_char = ATOM_INT32_CHAR,
    .kind      = 'i',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(int32_t),
    .alignment = _Alignof(int32_t),
    .name      = "int32",
};

// Unsigned 32-bit integer
static const Atom_DType g_atom_uint32_dtype = {
    .type_id   = ATOM_UINT32,
    .type_char = ATOM_UINT32_CHAR,
    .kind      = 'u',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(uint32_t),
    .alignment = _Alignof(uint32_t),
    .name      = "uint32",
};

// Signed 64-bit integer
static const Atom_DType g_atom_int64_dtype = {
    .type_id   = ATOM_INT64,
    .type_char = ATOM_INT64_CHAR,
    .kind      = 'i',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(int64_t),
    .alignment = _Alignof(int64_t),
    .name      = "int64",
};

// Unsigned 64-bit integer
static const Atom_DType g_atom_uint64_dtype = {
    .type_id   = ATOM_UINT64,
    .type_char = ATOM_UINT64_CHAR,
    .kind      = 'u',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(uint64_t),
    .alignment = _Alignof(uint64_t),
    .name      = "uint64",
};

// Float type
static const Atom_DType g_atom_float16_dtype = {
    .type_id   = ATOM_FLOAT16,
    .type_char = ATOM_FLOAT16_CHAR,
    .kind      = 'f',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(atom_float16_t),
    .alignment = _Alignof(atom_float16_t),
    .name      = "float16",
};

// 32-bit floating-point (single-precision)
static const Atom_DType g_atom_float32_dtype = {
    .type_id   = ATOM_FLOAT32,
    .type_char = ATOM_FLOAT32_CHAR,
    .kind      = 'f',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(float),
    .alignment = _Alignof(float),
    .name      = "float32",
};

// 64-bit floating-point (double-precision)
static const Atom_DType g_atom_float64_dtype = {
    .type_id   = ATOM_FLOAT64,
    .type_char = ATOM_FLOAT64_CHAR,
    .kind      = 'f',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(double),
    .alignment = _Alignof(double),
    .name      = "float64",
};

// Extended-precision floating-point (platform-dependent)
static const Atom_DType g_atom_longdouble_dtype = {
    .type_id   = ATOM_LONGDOUBLE,
    .type_char = ATOM_LONGDOUBLE_CHAR,
    .kind      = 'f',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(long double),
    .alignment = _Alignof(long double),
    .name      = "longdouble",
};

// Complex type
// Complex number made of two float32s

// 64-bit complex type
static const Atom_DType g_atom_cfloat64_dtype = {
     .type_id   = ATOM_CFLOAT64,
    .type_char = ATOM_CFLOAT64_CHAR,
    .kind      = 'c', // 'c' for complex
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(float) * 2,
    .alignment = _Alignof(float), // Alignment is based on the components
    .name      = "cfloat64",
};

// 128-bit complex type
static const Atom_DType g_atom_cfloat128_dtype = {
    .type_id   = ATOM_CFLOAT128,
    .type_char = ATOM_CFLOAT128_CHAR,
    .kind      = 'c',
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(double) * 2,
    .alignment = _Alignof(double),
    .name      = "cfloat128",
};

// --- Custom ML types ---

// Custom bfloat16 floating-point type
static const Atom_DType g_atom_bfloat16_dtype = {
    .type_id   = ATOM_BFLOAT16,
    .type_char = 'E',

    // We use 'V' (Void/Vendor-defined) for the kind, following the ml_dtypes
    // convention. This prevents a potential future conflict with a float16
    // type, which would also have itemsize=2 and a kind of 'f'.
    .kind      = 'V', 
    
    .byteorder = ATOM_BYTEORDER_NATIVE,
    .elsize    = sizeof(atom_bfloat16_t),
    .alignment = _Alignof(atom_bfloat16_t),
    .name      = "bfloat16",
};

// --- 2. The Internal Type Registry ---

// This array holds pointers to all the static type definitions.
// It is indexed by the Atom_DTypeID enum.
static const Atom_DType* const g_atom_dtypes[ATOM_NTYPES] = {
    // Boolean
    [ATOM_BOOL]   = &g_atom_bool_dtype,

    // Integer types
    [ATOM_INT8]   = &g_atom_int8_dtype,
    [ATOM_UINT8]  = &g_atom_uint8_dtype,
    [ATOM_INT16]  = &g_atom_int16_dtype,
    [ATOM_UINT16] = &g_atom_uint16_dtype,
    [ATOM_INT32]  = &g_atom_int32_dtype,
    [ATOM_UINT32] = &g_atom_uint32_dtype,
    [ATOM_INT64]  = &g_atom_int64_dtype,
    [ATOM_UINT64] = &g_atom_uint64_dtype,

    // Floating-point types
    [ATOM_FLOAT16]    = &g_atom_float16_dtype,
    [ATOM_FLOAT32]    = &g_atom_float32_dtype,
    [ATOM_FLOAT64]    = &g_atom_float64_dtype,
    [ATOM_LONGDOUBLE] = &g_atom_longdouble_dtype,

    // Complex types
    [ATOM_CFLOAT64]   = &g_atom_cfloat64_dtype,
    [ATOM_CFLOAT128]  = &g_atom_cfloat128_dtype,

    // Custom ML types
    [ATOM_BFLOAT16] = &g_atom_bfloat16_dtype,
};

// --- 3. The Public API Implementation ---

// Retrieves the canonical blueprint for a given type ID.
const Atom_DType* get_atom_dtype(Atom_DTypeID id) {
    // Safety check: ensure the requested ID is within the bounds of our registry.
    if (id >= ATOM_NTYPES) {
        return NULL;
    }
    // Return the pointer from our registry. If we haven't defined the type,
    // this will correctly return NULL.
    return g_atom_dtypes[id];
}