/*
 * dtype_object.h
 *
 * This header file declares the public interface for creating and registering
 * the `atom.dtype` Python type.
 */

#ifndef DTYPE_OBJECT_H
#define DTYPE_OBJECT_H

#include <Python.h>
#include "atom_types.h" // Also include this for Atom_DType definition

// --- NEW: Public definition of the Python object struct ---
// We move this here so that other C files (like atom_module.c) know
// what an AtomDTypeObject is and can use it.
typedef struct {
    PyObject_HEAD
    const Atom_DType* dtype_c;
} AtomDTypeObject;


/*
 * Initializes the `atom.dtype` type and adds it, along with all its pre-made
 * instances (e.g., atom.int32, atom.float64), to the main module.
 */
int init_atom_dtype_type(PyObject *module);


#endif // DTYPE_OBJECT_H