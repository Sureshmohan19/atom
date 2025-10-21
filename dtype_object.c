/*
 * =====================================================================================
 *
 *       Filename:  dtype_object.c
 *
 *    Description:  This file implements the `atom.dtype` Python type, including
 *                  its definition, behaviors (like repr and comparison),
 *                  and constructor.
 *
 * =====================================================================================
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

// Include our own headers
#include "atom_types.h"
#include "dtype_object.h" // Include its own header for the public function declaration

// =====================================================================================
//  1. DEFINE THE PYTHON-VISIBLE `atom.dtype` OBJECT
// =====================================================================================

static PyTypeObject AtomDType_Type; // Forward declaration

// =====================================================================================
//  2. DEFINE THE BEHAVIORS (SLOTS) FOR THE `atom.dtype` TYPE
// =====================================================================================

static PyObject*
atom_dtype_str(AtomDTypeObject *self) {
    if (self->dtype_c == NULL) {
        return PyUnicode_FromString("<atom.dtype NULL>");
    }
    return PyUnicode_FromString(self->dtype_c->name);
}

static PyObject*
atom_dtype_get_name(AtomDTypeObject *self, void *closure) {
    return PyUnicode_FromString(self->dtype_c->name);
}

static PyObject*
atom_dtype_get_elsize(AtomDTypeObject *self, void *closure) {
    return PyLong_FromSize_t(self->dtype_c->elsize);
}

static PyObject*
atom_dtype_get_alignment(AtomDTypeObject *self, void *closure) {
    return PyLong_FromSize_t(self->dtype_c->alignment);
}

static PyObject*
atom_dtype_get_char_attr(const char* attr_name, char attr_val) {
    char str[1] = { attr_val };
    return PyUnicode_FromStringAndSize(str, 1);
}

static PyObject*
atom_dtype_get_kind(AtomDTypeObject *self, void *closure) {
    return atom_dtype_get_char_attr("kind", self->dtype_c->kind);
}

static PyObject*
atom_dtype_get_type_char(AtomDTypeObject *self, void *closure) {
    return atom_dtype_get_char_attr("type_char", self->dtype_c->type_char);
}

static PyObject*
atom_dtype_get_byteorder(AtomDTypeObject *self, void *closure) {
    return atom_dtype_get_char_attr("byteorder", self->dtype_c->byteorder);
}

static PyGetSetDef atom_dtype_getsetters[] = {
    {"name", (getter)atom_dtype_get_name, NULL, "The common name of the data type", NULL},
    {"itemsize", (getter)atom_dtype_get_elsize, NULL, "The size of the data type in bytes", NULL},
    {"alignment", (getter)atom_dtype_get_alignment, NULL, "The required memory alignment", NULL},
    {"kind", (getter)atom_dtype_get_kind, NULL, "A character for the general kind of the type", NULL},
    {"type", (getter)atom_dtype_get_type_char, NULL, "A single character code for the type", NULL},
    {"char", (getter)atom_dtype_get_type_char, NULL, "A single character code for the type (alias for 'type')", NULL},
    {"byteorder", (getter)atom_dtype_get_byteorder, NULL, "A character indicating the byte order", NULL},
    {NULL}
};

static PyObject*
atom_dtype_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    if (!PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "dtype constructor expects a string");
        return NULL;
    }

    const char* dtype_name = PyUnicode_AsUTF8(obj);
    if (dtype_name == NULL) {
        return NULL;
    }

    const Atom_DType* found_dtype_c = NULL;
    for (Atom_DTypeID id = 0; id < ATOM_NTYPES; ++id) {
        const Atom_DType* dtype_c = get_atom_dtype(id);
        if (dtype_c != NULL && strcmp(dtype_c->name, dtype_name) == 0) {
            found_dtype_c = dtype_c;
            break;
        }
    }

    if (found_dtype_c == NULL) {
        PyErr_Format(PyExc_TypeError, "'%s' is not a valid Atom data type", dtype_name);
        return NULL;
    }

    AtomDTypeObject *self = (AtomDTypeObject *)subtype->tp_alloc(subtype, 0);
    if (self == NULL) {
        return NULL;
    }
    self->dtype_c = found_dtype_c;
    return (PyObject *)self;
}

static PyObject*
atom_dtype_richcompare(AtomDTypeObject *self, PyObject *other, int op) {
    if (!PyObject_TypeCheck(other, &AtomDType_Type)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    AtomDTypeObject *other_dtype = (AtomDTypeObject *)other;
    const Atom_DType* self_c = self->dtype_c;
    const Atom_DType* other_c = other_dtype->dtype_c;

    if (self_c == NULL || other_c == NULL) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    int are_equal = (self_c == other_c);
    switch (op) {
        case Py_EQ:
            if (are_equal) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        case Py_NE:
            if (!are_equal) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        default:
            Py_RETURN_NOTIMPLEMENTED;
    }
}

// =====================================================================================
//  3. DEFINE THE `atom.dtype` TYPE OBJECT
// =====================================================================================

static PyTypeObject AtomDType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "atom.dtype",
    .tp_basicsize = sizeof(AtomDTypeObject),
    .tp_itemsize = 0,
    .tp_repr = (reprfunc)atom_dtype_str,
    .tp_str  = (reprfunc)atom_dtype_str,
    .tp_getset = atom_dtype_getsetters,
    .tp_new = atom_dtype_new,
    .tp_richcompare = (richcmpfunc)atom_dtype_richcompare,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "An Atom data type object.",
};

// =====================================================================================
//  4. PUBLIC REGISTRATION FUNCTION
// =====================================================================================

// Helper function to add a single pre-made dtype instance to the module.
static int
add_dtype_to_module(PyObject *module, const Atom_DType *dtype_c) {
    AtomDTypeObject *dtype_py_obj = (AtomDTypeObject *)AtomDType_Type.tp_alloc(&AtomDType_Type, 0);
    if (dtype_py_obj == NULL) {
        return -1;
    }
    dtype_py_obj->dtype_c = dtype_c;
    if (PyModule_AddObject(module, dtype_c->name, (PyObject *)dtype_py_obj) < 0) {
        Py_DECREF(dtype_py_obj);
        return -1;
    }
    return 0;
}

// This is the public function that will be called by `atom_module.c`.
int init_atom_dtype_type(PyObject *module) {
    // Finalize the `atom.dtype` type object.
    if (PyType_Ready(&AtomDType_Type) < 0) {
        return -1;
    }

    // Add the `dtype` type itself to the module, so users can call `atom.dtype(...)`.
    Py_INCREF(&AtomDType_Type);
    if (PyModule_AddObject(module, "dtype", (PyObject *)&AtomDType_Type) < 0) {
        Py_DECREF(&AtomDType_Type);
        return -1;
    }

    // Loop through all our C types and add the pre-made instances.
    for (Atom_DTypeID id = 0; id < ATOM_NTYPES; ++id) {
        const Atom_DType* dtype_c = get_atom_dtype(id);
        if (dtype_c != NULL) {
            if (add_dtype_to_module(module, dtype_c) < 0) {
                // On failure, the error is already set.
                return -1;
            }
        }
    }

    return 0; // Success
}