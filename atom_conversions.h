/*
 * atom_conversions.h
 *
 * Declares functions for converting between standard C types and Atom's
 * custom data types, such as bfloat16.
 */

#ifndef ATOM_CONVERSIONS_H
#define ATOM_CONVERSIONS_H

#include "atom_types.h" // Required for the atom_bfloat16_t typedef

// --- BFloat16 Conversion Functions ---

/*
 * Converts a standard 32-bit float to a 16-bit bfloat16.
 *
 * This implementation is a direct C translation of the robust "round-to-nearest-even"
 * algorithm used in libraries like Eigen and TensorFlow. It handles NaN and
 * rounding correctly for improved numerical stability.
 *
 * @param f The input float.
 * @return The resulting bfloat16 value, stored in a uint16_t.
 */
atom_bfloat16_t atom_float32_to_bfloat16(float f);


/*
 * Converts a 16-bit bfloat16 back to a 32-bit float.
 *
 * This conversion is lossless. The logic involves shifting the 16 bits of the
 * bfloat16 into the upper (exponent and high-mantissa) bits of a 32-bit
 * integer, and setting the lower 16 bits to zero.
 *
 * @param bf The input bfloat16 value.
 * @return The resulting 32-bit float.
 */
float atom_bfloat16_to_float32(atom_bfloat16_t bf);


#endif // ATOM_CONVERSIONS_H