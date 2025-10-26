/*
 * =====================================================================================
 *
 *       Filename:  atom_module.c
 *
 *    Description:  This file is the main entry point for the 'atom' Python module.
 *                  It defines the module and its top-level functions (like finfo, iinfo)
 *                  and orchestrates the initialization of custom Python types
 *                  defined in other files.
 *
 * =====================================================================================
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

// --- ADD NUMPY CONFIGURATION MACROS HERE ---
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL ATOM_NUMPY_API
// We only want to import the array in this one file.
// So, DO NOT define NO_IMPORT_ARRAY here.
#include <numpy/arrayobject.h>

#include "atom_types.h"
#include <float.h>
#include <limits.h>

// --- Include headers for our component files ---
#include "dtype_object.h" // Our new header for the dtype object functionality.


// =====================================================================================
//  1. DEFINE `finfo` and `iinfo` OBJECTS AND FUNCTIONS
// =====================================================================================

// --- finfo Object and Type Definitions ---
typedef struct {
    PyObject_HEAD
    int bits;
    double eps;
    double max;
    double min;
    int precision;
    double resolution;
    PyObject *dtype;
    int nmant;    // Number of mantissa bits
    int nexp;     // Number of exponent bits
    int minexp;   // Minimum exponent
    int maxexp;   // Maximum exponent
    double epsneg;
    int machep;
    int negep;
    double tiny;
    double smallest_normal;
    double smallest_subnormal;
} AtomFInfoObject;

// NEW deallocator for finfo
static void
atom_finfo_dealloc(AtomFInfoObject *self) {
    Py_XDECREF(self->dtype); // Release our reference to the dtype object.
    Py_TYPE(self)->tp_free((PyObject *)self); // Free the finfo object itself.
}

static PyMemberDef atom_finfo_members[] = {
    {"bits", T_INT, offsetof(AtomFInfoObject, bits), READONLY, "number of bits in the type"},
    {"eps", T_DOUBLE, offsetof(AtomFInfoObject, eps), READONLY, "machine epsilon"},
    {"max", T_DOUBLE, offsetof(AtomFInfoObject, max), READONLY, "largest representable number"},
    {"min", T_DOUBLE, offsetof(AtomFInfoObject, min), READONLY, "smallest positive normalized number"},
    {"precision", T_INT, offsetof(AtomFInfoObject, precision), READONLY, "approximate number of decimal digits of precision"},
    {"resolution", T_DOUBLE, offsetof(AtomFInfoObject, resolution), READONLY, "approximate decimal resolution"},
    {"dtype", T_OBJECT_EX, offsetof(AtomFInfoObject, dtype), READONLY, "the dtype object this finfo is for"},
    {"nmant", T_INT, offsetof(AtomFInfoObject, nmant), READONLY, "number of bits in the mantissa"},
    {"nexp", T_INT, offsetof(AtomFInfoObject, nexp), READONLY, "number of bits in the exponent"},
    {"minexp", T_INT, offsetof(AtomFInfoObject, minexp), READONLY, "minimum negative exponent"},
    {"maxexp", T_INT, offsetof(AtomFInfoObject, maxexp), READONLY, "maximum positive exponent"},
    {"epsneg", T_DOUBLE, offsetof(AtomFInfoObject, epsneg), READONLY, "epsilon for negative rounding"},
    {"machep", T_INT, offsetof(AtomFInfoObject, machep), READONLY, "exponent that yields eps"},
    {"negep", T_INT, offsetof(AtomFInfoObject, negep), READONLY, "exponent that yields epsneg"},
    {"tiny", T_DOUBLE, offsetof(AtomFInfoObject, tiny), READONLY, "alias for smallest_normal"},
    {"smallest_normal", T_DOUBLE, offsetof(AtomFInfoObject, smallest_normal), READONLY, "smallest positive normal value"},
    {"smallest_subnormal", T_DOUBLE, offsetof(AtomFInfoObject, smallest_subnormal), READONLY, "smallest positive subnormal value"},
    {NULL}
};

static PyTypeObject AtomFInfo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "atom.finfo",
    .tp_basicsize = sizeof(AtomFInfoObject),
    .tp_dealloc = (destructor)atom_finfo_dealloc,
    .tp_itemsize = 0,
    .tp_members = atom_finfo_members,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "finfo(dtype) -> finfo object\n\nObject containing floating point type information.",
};

// --- iinfo Object and Type Definitions ---
typedef struct {
    PyObject_HEAD
    int bits;
    int64_t min;
    uint64_t max;
    PyObject *dtype;
} AtomIInfoObject;

static void
atom_iinfo_dealloc(AtomIInfoObject *self) {
    Py_XDECREF(self->dtype); // Release our reference to the dtype object.
    Py_TYPE(self)->tp_free((PyObject *)self); // Free the iinfo object itself.
}

static PyMemberDef atom_iinfo_members[] = {
    {"bits", T_INT, offsetof(AtomIInfoObject, bits), READONLY, "number of bits in the type"},
    {"min", T_LONGLONG, offsetof(AtomIInfoObject, min), READONLY, "minimum value of the type"},
    {"max", T_ULONGLONG, offsetof(AtomIInfoObject, max), READONLY, "maximum value of the type"},
    {"dtype", T_OBJECT_EX, offsetof(AtomIInfoObject, dtype), READONLY, "the dtype object this iinfo is for"},
    {NULL}
};

static PyTypeObject AtomIInfo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "atom.iinfo",
    .tp_basicsize = sizeof(AtomIInfoObject),
    .tp_dealloc = (destructor)atom_iinfo_dealloc,
    .tp_itemsize = 0,
    .tp_members = atom_iinfo_members,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "iinfo(dtype) -> iinfo object\n\nObject containing integer type information.",
};


// --- Implementation of the atom.finfo() function ---
static PyObject*
atom_module_finfo(PyObject *self, PyObject *args) {
    PyObject* dtype_arg;
    if (!PyArg_ParseTuple(args, "O", &dtype_arg)) return NULL;
    
    // We can't use AtomDType_Type here because it's defined in another file.
    // We would need to expose it in the header if we wanted to type check.
    // For now, we rely on the kind check.

    AtomDTypeObject *dtype_obj = (AtomDTypeObject *)dtype_arg;
    const Atom_DType* dtype_c = dtype_obj->dtype_c;

    if (dtype_c->kind != 'f' && dtype_c->kind != 'c' && dtype_c->kind != 'V') {
        PyErr_SetString(PyExc_TypeError, "finfo is only available for floating point and complex dtypes, try iinfo for integers.");
        return NULL;
    }
    
    AtomFInfoObject *finfo_obj = (AtomFInfoObject *)AtomFInfo_Type.tp_alloc(&AtomFInfo_Type, 0);
    if (finfo_obj == NULL) return NULL;

    // Store the dtype object ---
    Py_INCREF(dtype_arg); // We are keeping a reference to it, so we must increment its ref count.
    finfo_obj->dtype = dtype_arg;
    
    switch (dtype_c->type_id) {
        case ATOM_FLOAT16:
            finfo_obj->bits = 16;
            finfo_obj->eps = 0.0009765625;      // 2**-10
            finfo_obj->max = 65504.0;
            finfo_obj->min = -65504.0;
            finfo_obj->precision = 3;           // floor(10 * log10(2))
            finfo_obj->resolution = 1e-3;
            finfo_obj->nmant = 10;
            finfo_obj->nexp = 5;
            finfo_obj->minexp = -14;
            finfo_obj->maxexp = 16;
            finfo_obj->epsneg = 0.00048828125;  // 2**-11
            finfo_obj->machep = -10;
            finfo_obj->negep = -11;
            finfo_obj->tiny = 6.1035156e-05;    // 2**-14
            finfo_obj->smallest_normal = 6.1035156e-05;
            finfo_obj->smallest_subnormal = 5.9604645e-08; // 2**-24
            break;
        case ATOM_FLOAT32:
        case ATOM_CFLOAT64:
            finfo_obj->bits = 32; 
            finfo_obj->eps = FLT_EPSILON; 
            finfo_obj->max = FLT_MAX;
            finfo_obj->min = -FLT_MAX; 
            finfo_obj->precision = 6; 
            finfo_obj->resolution = 1e-6;
            finfo_obj->nmant = 23; 
            finfo_obj->nexp = 8;
            finfo_obj->minexp = -126; 
            finfo_obj->maxexp = 128;
            finfo_obj->epsneg = FLT_EPSILON / 2.0;
            finfo_obj->machep = -23;
            finfo_obj->negep = -24;
            finfo_obj->tiny = FLT_MIN;
            finfo_obj->smallest_normal = FLT_MIN;
            finfo_obj->smallest_subnormal = FLT_TRUE_MIN; // May need a fallback if not defined
            break;
        case ATOM_FLOAT64:
        case ATOM_CFLOAT128:
            finfo_obj->bits = 64; 
            finfo_obj->eps = DBL_EPSILON; 
            finfo_obj->max = DBL_MAX;
            finfo_obj->min = -DBL_MAX; 
            finfo_obj->precision = 15; 
            finfo_obj->resolution = 1e-15;
            finfo_obj->nmant = 52; 
            finfo_obj->nexp = 11;
            finfo_obj->minexp = -1022; 
            finfo_obj->maxexp = 1024;
             finfo_obj->epsneg = DBL_EPSILON / 2.0;
            finfo_obj->machep = -52;
            finfo_obj->negep = -53;
            finfo_obj->tiny = DBL_MIN;
            finfo_obj->smallest_normal = DBL_MIN;
            finfo_obj->smallest_subnormal = DBL_TRUE_MIN; // May need a fallback
            break;
        case ATOM_BFLOAT16:
            finfo_obj->bits = 16; 
            finfo_obj->eps = 0.0078125; 
            finfo_obj->max = 3.389531e+38;
            finfo_obj->min = -3.389531e+38; 
            finfo_obj->precision = 2; 
            finfo_obj->resolution = 1e-2;
            finfo_obj->nmant = 7; 
            finfo_obj->nexp = 8;
            finfo_obj->minexp = -126; 
            finfo_obj->maxexp = 128;
            finfo_obj->epsneg = 0.0078125 / 2.0; // eps / 2
            finfo_obj->machep = -7;
            finfo_obj->negep = -8;
            finfo_obj->tiny = 1.17549435e-38; // 2**-126
            finfo_obj->smallest_normal = 1.17549435e-38; // Same as tiny
            finfo_obj->smallest_subnormal = 9.18355e-41; // 2**-133
            
            break;
        default:
            Py_DECREF(finfo_obj);
            PyErr_SetString(PyExc_TypeError, "finfo not available for this type.");
            return NULL;
    }
    return (PyObject *)finfo_obj;
}


// --- Implementation of the atom.iinfo() function ---
static PyObject*
atom_module_iinfo(PyObject *self, PyObject *args) {
    PyObject* dtype_arg;
    if (!PyArg_ParseTuple(args, "O", &dtype_arg)) return NULL;

    // We can't type check here without access to AtomDType_Type
    AtomDTypeObject *dtype_obj = (AtomDTypeObject *)dtype_arg;
    const Atom_DType* dtype_c = dtype_obj->dtype_c;

    if (dtype_c->kind != 'i' && dtype_c->kind != 'u' && dtype_c->kind != 'b') {
        PyErr_SetString(PyExc_TypeError, "iinfo is only available for integer and boolean dtypes, try finfo for floats.");
        return NULL;
    }
    
    AtomIInfoObject *iinfo_obj = (AtomIInfoObject *)AtomIInfo_Type.tp_alloc(&AtomIInfo_Type, 0);
    if (iinfo_obj == NULL) return NULL;

    // Store the dtype object ---
    Py_INCREF(dtype_arg); // We are keeping a reference to it, so we must increment its ref count.
    iinfo_obj->dtype = dtype_arg;
    
    iinfo_obj->bits = dtype_c->elsize * 8;
    switch (dtype_c->type_id) {
        case ATOM_BOOL:   iinfo_obj->min = 0; iinfo_obj->max = 1; break;
        case ATOM_INT8:   iinfo_obj->min = INT8_MIN; iinfo_obj->max = INT8_MAX; break;
        case ATOM_UINT8:  iinfo_obj->min = 0; iinfo_obj->max = UINT8_MAX; break;
        case ATOM_INT16:  iinfo_obj->min = INT16_MIN; iinfo_obj->max = INT16_MAX; break;
        case ATOM_UINT16: iinfo_obj->min = 0; iinfo_obj->max = UINT16_MAX; break;
        case ATOM_INT32:  iinfo_obj->min = INT32_MIN; iinfo_obj->max = INT32_MAX; break;
        case ATOM_UINT32: iinfo_obj->min = 0; iinfo_obj->max = UINT32_MAX; break;
        case ATOM_INT64:  iinfo_obj->min = INT64_MIN; iinfo_obj->max = INT64_MAX; break;
        case ATOM_UINT64: iinfo_obj->min = 0; iinfo_obj->max = UINT64_MAX; break;
        default:
            Py_DECREF(iinfo_obj);
            PyErr_SetString(PyExc_TypeError, "iinfo not available for this type.");
            return NULL;
    }
    return (PyObject *)iinfo_obj;
}

// =====================================================================================
//  2. DEFINE THE `atom` MODULE AND ITS INITIALIZATION
// =====================================================================================

static PyMethodDef atom_module_methods[] = {
    {"finfo", atom_module_finfo, METH_VARARGS, "finfo(dtype) -> finfo object\n\nGet information about a floating point data type."},
    {"iinfo", atom_module_iinfo, METH_VARARGS, "iinfo(dtype) -> iinfo object\n\nGet information about an integer data type."},

    {NULL, NULL, 0, NULL} // Sentinel
};

static struct PyModuleDef atom_module_definition = {
    PyModuleDef_HEAD_INIT,
    "atom",
    "A library of fundamental data types built from scratch in C.",
    -1,
    atom_module_methods
};

/*
 * This is the official entry point for the module.
 */
PyMODINIT_FUNC PyInit_atom(void) {
    PyObject *module;

    // --- Initialize the NumPy C-API ---
    // The import_array() macro must be called here. On failure, it
    // will execute `return NULL;` automatically.
    import_array();

    // Create the main module object.
    module = PyModule_Create(&atom_module_definition);
    if (module == NULL) {
        return NULL;
    }

    // Call the registration function from our other C file to add the dtype object.
    if (init_atom_dtype_type(module) < 0) {
        Py_DECREF(module);
        return NULL;
    }

    // Finalize the `finfo` and `iinfo` types. They don't need to be added
    // to the module directly, as they are only returned by functions.
    if (PyType_Ready(&AtomFInfo_Type) < 0) {
        Py_DECREF(module);
        return NULL;
    }
    if (PyType_Ready(&AtomIInfo_Type) < 0) {
        Py_DECREF(module);
        return NULL;
    }

    // Return the fully initialized and populated module.
    return module;
}