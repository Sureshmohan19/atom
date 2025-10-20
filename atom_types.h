/*
 * atom_types.h
 *
 * This header file defines the fundamental data types for the Atom library.
 * It establishes a "blueprint" struct for each type, specifying its static
 * properties like size and alignment. This is the public interface for the
 * Atom type system.
 */

#ifndef ATOM_TYPES_H
#define ATOM_TYPES_H

#include <stddef.h> 
#include <stdint.h> 

// C has no native 16-bit float, so we use a 16-bit unsigned integer
// to hold the raw bit pattern.
typedef uint16_t atom_bfloat16_t;

/*
 * A unique integer ID for every data type in the Atom library.
 * This provides a fast, machine-readable way to identify types.
 */
typedef enum Atom_DTypeID {
    ATOM_BOOL=0,

    // --- Standard Integer Types ---
    ATOM_INT8,       
    ATOM_UINT8,      
    ATOM_INT16,
    ATOM_UINT16,
    ATOM_INT32,
    ATOM_UINT32,
    ATOM_INT64,
    ATOM_UINT64,

    // --- Standard Floating-Point Types ---
    /*
     * Deferred: ATOM_FLOAT16 is not a native C type. It requires a custom
     * typedef (e.g., `typedef uint16_t atom_float16_t;`) to define its
     * storage. We will handle it later with other non-native types.
     *
     * ATOM_FLOAT16,
     */

    ATOM_FLOAT32,    
    ATOM_FLOAT64,    
    ATOM_LONGDOUBLE,

    // --- Standard Complex Types ---
    ATOM_CFLOAT64,   
    ATOM_CFLOAT128,

    /*
     * Deferred Types: We will define the following types later.
     *
     * ATOM_CLONGDOUBLE,
     * ATOM_STRING,
     * ATOM_UNICODE,
     * ATOM_VOID,
     * ATOM_DATETIME,
     * ATOM_TIMEDELTA,
     * ATOM_OBJECT,
     */

    /*
     * Deferred Custom ML Types: New, non-standard types specifically
     * for machine learning will be declared here later.
     *
     * Examples:
     * ATOM_BFLOAT16, // Brain Floating Point
     * ATOM_INT4,     // 4-bit integer
     * ATOM_E4M3,     // 8-bit float with 4-bit exponent, 3-bit mantissa
     * ATOM_E5M2,     // 8-bit float with 5-bit exponent, 2-bit mantissa
     */

    // --- Custom ML Types ---
    // We are now defining our first custom type.
    ATOM_BFLOAT16,

    // --- Sentinel ---
    // This value automatically tracks the number of defined types.
    ATOM_NTYPES

} Atom_DTypeID;

/*
 * The single-character code for each data type.
 * This is for compatibility with NumPy's conventions and for string representations.
 */
typedef enum {
    // --- Integer Type Characters ---
    ATOM_BOOL_CHAR = '?',   
    ATOM_INT8_CHAR = 'b',   
    ATOM_UINT8_CHAR = 'B',  
    ATOM_INT16_CHAR = 'h',  
    ATOM_UINT16_CHAR = 'H', 
    ATOM_INT32_CHAR = 'i', 
    ATOM_UINT32_CHAR = 'I', 
    ATOM_INT64_CHAR = 'q',  
    ATOM_UINT64_CHAR = 'Q', 

    // --- Floating-Point Type Characters ---
    // ATOM_FLOAT16_CHAR = 'e', is deferred   
    ATOM_FLOAT32_CHAR = 'f',    
    ATOM_FLOAT64_CHAR = 'd',    
    ATOM_LONGDOUBLE_CHAR = 'g', 

    // --- Complex Type Characters ---
    ATOM_CFLOAT64_CHAR = 'F',  
    ATOM_CFLOAT128_CHAR = 'D', 

} Atom_TypeChar;

/*
 * Defines the byte order (endianness) of a multi-byte data type.
 */
typedef enum {
    // Not applicable (e.g., for a 1-byte type like int8).
    ATOM_BYTEORDER_NA = '|',

    // Native byte order of the machine the code is compiled on.
    ATOM_BYTEORDER_NATIVE = '=',

    // Little-endian (least significant byte first).
    ATOM_BYTEORDER_LE = '<',

    // Big-endian (most significant byte first).
    ATOM_BYTEORDER_BE = '>',
} Atom_ByteOrder;

/*
 * The "blueprint" structure that defines the static properties of a data type.
 * Each instance of this struct describes one unique type in the Atom library.
 */
typedef struct {
    // The unique integer ID from the Atom_DTypeID enum.
    Atom_DTypeID type_id;

    // The unique single-character code for the type (e.g., 'i', 'f').
    char type_char;

    // The general "kind" of the type (e.g., 'i' for signed, 'u' for unsigned, 'f' for float).
    char kind;

    // The byte order (endianness) of the type.
    char byteorder;

    // The size of a single element of this type in bytes.
    size_t elsize;

    // The required memory alignment for this type in bytes.
    size_t alignment;

    // A human-readable name for the type (e.g., "int32", "float64").
    const char *name;

} Atom_DType;

/*
 * C-API: Retrieves a pointer to the canonical, read-only blueprint for a given type ID.
 * This is the primary way to access type information in the Atom library.
 */
const Atom_DType* get_atom_dtype(Atom_DTypeID id);

#endif