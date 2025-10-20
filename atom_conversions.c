/*
 * atom_conversions.c
 *
 * Implements the conversion logic for Atom's custom data types.
 */

#include "atom_conversions.h"
#include <math.h> // Required for isnan()

// A helper union for re-interpreting the bits of a float as an integer.
// This is the standard C way to perform this type of bit-level manipulation.
typedef union {
    float f32;
    uint32_t u32;
} float_caster;


atom_bfloat16_t atom_float32_to_bfloat16(float f) {
    float_caster converter;
    converter.f32 = f;

    // This logic is a direct C translation of Eigen's `float_to_bfloat16_rtne`.

    // Handle NaN case separately to ensure the result is a quiet NaN.
    if (isnan(f)) {
        // The result of the shift might be a signaling NaN, so we OR with
        // 0x0040 to ensure the most significant bit of the mantissa is set,
        // making it a quiet NaN, which is standard.
        return (atom_bfloat16_t)((converter.u32 >> 16) | 0x0040);
    }

    // This is the core of the round-to-nearest-even algorithm.
    // 1. Get the least significant bit of the bfloat16 that will be kept.
    uint32_t lsb = (converter.u32 >> 16) & 1;

    // 2. Calculate the rounding bias. If the LSB is 1, we add one more to
    //    the bias to ensure rounding to an even number in the case of a tie.
    uint32_t rounding_bias = 0x00007FFF + lsb;

    // 3. Add the bias to the original 32-bit float's integer representation.
    //    This addition correctly rounds up in cases of > 0.5, and handles
    //    the rounding-to-even case for exactly 0.5.
    converter.u32 += rounding_bias;

    // 4. Truncate by shifting right by 16 bits.
    return (atom_bfloat16_t)(converter.u32 >> 16);
}


float atom_bfloat16_to_float32(atom_bfloat16_t bf) {
    float_caster converter;

    // The conversion back to float is simpler. We place the 16 bits of the
    // bfloat into the upper 16 bits of a 32-bit unsigned integer, leaving
    // the lower 16 bits (the lost part of the mantissa) as zero.
    converter.u32 = (uint32_t)bf << 16;

    // Re-interpret the resulting 32 bits as a float.
    return converter.f32;
}