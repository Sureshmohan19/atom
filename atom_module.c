/*
 * =====================================================================================
 *
 *       Filename:  atom_module.c
 *
 *    Description:  This file serves as the bridge between our pure C 'Atom' type
 *                  library and the Python interpreter. It uses the Python C-API
 *                  to create a new Python module named 'atom' and its related functions
 *
 * =====================================================================================
 */

// This macro is required on some platforms to ensure that Python's C-API
// uses the correct, modern definitions for types like `Py_ssize_t`. It helps
// maintain forward compatibility.
#define PY_SSIZE_T_CLEAN
#include <Python.h>      // The main header for the Python C-API. It's our gateway to all Python functions and types.
#include "structmember.h" // Required for the `PyMemberDef` and `PyGetSetDef` structures, which we use to expose C struct members as Python attributes.

// This is our own library. We include its public header to gain access to the
// `Atom_DType` struct definitions and the `get_atom_dtype` C-API function.
#include "atom_types.h"

// For FLT_MAX, DBL_MAX, etc.
#include <float.h> 

// =====================================================================================
//  1. DEFINE THE PYTHON-VISIBLE `atom.dtype` OBJECT
// =====================================================================================

/*
 * This is the C struct that will be the in-memory representation of every
 * `atom.dtype` object in Python. It acts as a lightweight Python wrapper
 * around our pure C `Atom_DType` blueprint.
 */
typedef struct {
    // This MUST be the first member of any C struct that represents a Python
    // object. It contains the reference count and a pointer to the object's
    // type, which are essential for Python's memory management and type system.
    PyObject_HEAD

    // This is our custom payload. It's a pointer to the static, read-only
    // `Atom_DType` struct (e.g., `g_atom_int8_dtype`) from our C library.
    // This is how we link a specific Python object to its underlying C definition.
    const Atom_DType* dtype_c;
} AtomDTypeObject;

/*
 * This is a "forward declaration". It's a promise to the C compiler that a
 * global variable named `AtomDType_Type` with the type `PyTypeObject` will be
 * fully defined later in this file. This is necessary because some of our
 * functions (like `atom_dtype_richcompare`) need to refer to `AtomDType_Type`
 * before the compiler has seen its full definition.
 */
static PyTypeObject AtomDType_Type;


// =====================================================================================
//  2. DEFINE THE BEHAVIORS (SLOTS) FOR THE `atom.dtype` TYPE
// =====================================================================================

/*
 * Implements the `__str__` and `__repr__` behavior for our `atom.dtype` object.
 * Python calls this function whenever it needs a user-friendly string
 * representation of the object (e.g., for `print()`).
 *
 * `self`: A pointer to the specific `AtomDTypeObject` instance being printed.
 * Returns: A new reference to a Python Unicode (string) object.
 */
static PyObject*
atom_dtype_str(AtomDTypeObject *self)
{
    // A defensive check. If the internal C pointer is NULL, we should return
    // a safe representation instead of crashing.
    if (self->dtype_c == NULL) {
        return PyUnicode_FromString("<atom.dtype NULL>");
    }

    // `PyUnicode_FromString` is a C-API function that takes a standard C
    // null-terminated string and creates a new Python string object from it.
    // We get the name directly from our linked C blueprint.
    return PyUnicode_FromString(self->dtype_c->name);
}

/*
 * --- Getter Functions for Python Attributes ---
 *
 * The following group of functions implements the "getter" part of Python's
 * attribute access. When you write `my_dtype.itemsize` in Python, the
 * interpreter will call the C function associated with "itemsize"
 * to get the value.
 */

static PyObject*
atom_dtype_get_name(AtomDTypeObject *self, void *closure) {
    return PyUnicode_FromString(self->dtype_c->name);
}

static PyObject*
atom_dtype_get_elsize(AtomDTypeObject *self, void *closure) {
    // `PyLong_FromSize_t` correctly converts a C `size_t` into a Python integer object.
    return PyLong_FromSize_t(self->dtype_c->elsize);
}

static PyObject*
atom_dtype_get_alignment(AtomDTypeObject *self, void *closure) {
    return PyLong_FromSize_t(self->dtype_c->alignment);
}

// This is a small helper to avoid writing the same code for all single-character attributes.
static PyObject*
atom_dtype_get_char_attr(const char* attr_name, char attr_val) {
    char str[1] = { attr_val };
    // We create a Python string of length 1 from our C char.
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

/*
 * This is the "get/set" definition table. It maps Python attribute names to
 * the C getter (and optionally setter) functions that should be called.
 * It's an array of `PyGetSetDef` structs, terminated by a NULL entry.
 */
static PyGetSetDef atom_dtype_getsetters[] = {
    // Each entry defines one attribute.
    {
        "name",                          // The name of the attribute in Python.
        (getter)atom_dtype_get_name,     // A pointer to the C getter function.
        NULL,                            // The setter function (NULL means it's read-only).
        "The common name of the data type (e.g., 'int32')", // The attribute's docstring.
        NULL                             // "closure" data, not needed here.
    },
    {"itemsize",  (getter)atom_dtype_get_elsize,    NULL, "The size of the data type in bytes", NULL},
    {"alignment", (getter)atom_dtype_get_alignment, NULL, "The required memory alignment", NULL},
    {"kind",      (getter)atom_dtype_get_kind,      NULL, "A character for the general kind of the type", NULL},
    {"type",      (getter)atom_dtype_get_type_char, NULL, "A single character code for the type", NULL},
    {"char",      (getter)atom_dtype_get_type_char, NULL, "A single character code for the type (alias for 'type')", NULL},
    {"byteorder", (getter)atom_dtype_get_byteorder, NULL, "A character indicating the byte order", NULL},

    // This sentinel entry marks the end of the array and is mandatory.
    {NULL}
};

/*
 * Implements the `__new__` method, which is the constructor for our type.
 * Python calls this when a user writes `atom.dtype(...)`.
 *
 * `subtype`: A pointer to the type being created (in this case, `AtomDType_Type`).
 * `args`: A Python tuple of the positional arguments.
 * `kwds`: A Python dict of the keyword arguments (we don't use this yet).
 * Returns: A new reference to an `AtomDTypeObject` instance.
 */
static PyObject*
atom_dtype_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
    PyObject *obj;
    // `PyArg_ParseTuple` is the standard way to parse arguments passed from Python.
    // The "O" format code means "get one Python Object".
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL; // `PyArg_ParseTuple` sets the Python error on failure.
    }

    // We only support creating dtypes from strings for now.
    if (!PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "dtype constructor expects a string");
        return NULL;
    }

    // Convert the Python Unicode object into a C-style `const char*`.
    const char* dtype_name = PyUnicode_AsUTF8(obj);
    if (dtype_name == NULL) {
        return NULL; // `PyUnicode_AsUTF8` sets the error on failure.
    }

    // --- Search our C library for a matching type ---
    const Atom_DType* found_dtype_c = NULL;
    for (Atom_DTypeID id = 0; id < ATOM_NTYPES; ++id) {
        const Atom_DType* dtype_c = get_atom_dtype(id);
        if (dtype_c != NULL && strcmp(dtype_c->name, dtype_name) == 0) {
            found_dtype_c = dtype_c;
            break; // Found it!
        }
    }

    if (found_dtype_c == NULL) {
        // `PyErr_Format` is like `printf` but for setting Python exceptions.
        PyErr_Format(PyExc_TypeError, "'%s' is not a valid Atom data type", dtype_name);
        return NULL;
    }

    // --- Create and return the new Python wrapper object ---
    // `subtype->tp_alloc` is the standard way to allocate memory for a new Python object instance.
    AtomDTypeObject *self = (AtomDTypeObject *)subtype->tp_alloc(subtype, 0);
    if (self == NULL) {
        return NULL; // `tp_alloc` sets a MemoryError if it fails.
    }

    self->dtype_c = found_dtype_c; // Link the new Python object to the C blueprint.

    return (PyObject *)self;
}

/*
 * Implements rich comparison logic (==, !=, <, >, etc.).
 * Python calls this function for operations like `atom.int32 == atom.dtype('int32')`.
 *
 * `self`: The left-hand side of the comparison.
 * `other`: The right-hand side of the comparison.
 * `op`: An integer ID representing the operation (e.g., `Py_EQ` for `==`).
 * Returns: `Py_True`, `Py_False`, or `Py_NotImplemented`.
 */
static PyObject*
atom_dtype_richcompare(AtomDTypeObject *self, PyObject *other, int op)
{
    // We only know how to compare ourselves to other `atom.dtype` objects.
    if (!PyObject_TypeCheck(other, &AtomDType_Type)) {
        // `Py_RETURN_NOTIMPLEMENTED` is a macro that returns the special
        // `NotImplemented` singleton. This allows Python to try other ways
        // to complete the comparison (e.g., asking `other` if it knows how).
        Py_RETURN_NOTIMPLEMENTED;
    }

    AtomDTypeObject *other_dtype = (AtomDTypeObject *)other;

    const Atom_DType* self_c = self->dtype_c;
    const Atom_DType* other_c = other_dtype->dtype_c;

    if (self_c == NULL || other_c == NULL) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    // The core logic: two dtypes are "equal" if they point to the exact same
    // static C blueprint struct. This is a very fast and effective pointer comparison.
    int are_equal = (self_c == other_c);

    // `Py_EQ` means `==` and `Py_NE` means `!=`.
    switch (op) {
        case Py_EQ:
            if (are_equal) Py_RETURN_TRUE; // Macro for `Py_INCREF(Py_True); return Py_True;`
            else Py_RETURN_FALSE;         // Macro for `Py_INCREF(Py_False); return Py_False;`
        case Py_NE:
            if (!are_equal) Py_RETURN_TRUE;
            else Py_RETURN_FALSE;
        default:
            // We don't define an ordering (<, >), so we can't handle other cases.
            Py_RETURN_NOTIMPLEMENTED;
    }
}


// =====================================================================================
//  3. DEFINE THE `atom.dtype` TYPE OBJECT
// =====================================================================================

/*
 * This is the master definition of our new Python type. It's a large, static
 * struct where we "plug in" all the behavior functions we wrote above into
 * specific "slots" that the Python interpreter understands.
 */
static PyTypeObject AtomDType_Type = {
    // This macro initializes the header fields required for all Python objects.
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "atom.dtype",                // The full name of the type, as seen in Python.
    .tp_basicsize = sizeof(AtomDTypeObject),// The size in bytes of our C wrapper struct.
    .tp_itemsize = 0,                       // For variable-sized objects (not us).

    // --- Behavior Slots ---
    .tp_repr = (reprfunc)atom_dtype_str,    // Function to call for `repr()`.
    .tp_str  = (reprfunc)atom_dtype_str,    // Function to call for `str()` and `print()`.
    .tp_getset = atom_dtype_getsetters,     // The table of our attribute getters.
    .tp_new = atom_dtype_new,               // The constructor function.
    .tp_richcompare = (richcmpfunc)atom_dtype_richcompare, // The comparison function.

    // --- Flags ---
    // `Py_TPFLAGS_DEFAULT` is a standard set of flags for a well-behaved type.
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "An Atom data type object.",  // The docstring for the type.
};

/*
 * Helper function: Creates a Python AtomDTypeObject from a C Atom_DType blueprint
 * and adds it to the given module.
 *
 * THIS FUNCTION MUST BE DEFINED BEFORE PyInit_atom, WHICH CALLS IT.
 *
 * Returns 0 on success, -1 on failure.
 */
static int
add_dtype_to_module(PyObject *module, const Atom_DType *dtype_c)
{
    // Allocate a new Python object of our custom type (AtomDTypeObject).
    AtomDTypeObject *dtype_py_obj = (AtomDTypeObject *)AtomDType_Type.tp_alloc(&AtomDType_Type, 0);
    if (dtype_py_obj == NULL) {
        return -1; // Allocation failed
    }

    // Link the new Python object to our existing C blueprint struct.
    dtype_py_obj->dtype_c = dtype_c;

    // Add the new Python object to the module dictionary.
    // The name it gets in Python is taken directly from the C blueprint's 'name' field.
    // PyModule_AddObject steals a reference to dtype_py_obj on success.
    if (PyModule_AddObject(module, dtype_c->name, (PyObject *)dtype_py_obj) < 0) {
        // If adding fails, we are responsible for decrementing the reference.
        Py_DECREF(dtype_py_obj);
        return -1;
    }

    return 0; // Success
}

// --- NEW: finfo Object and Type Definitions ---

/*
 * This is the C struct that will be the in-memory representation of an
 * `atom.finfo` object in Python.
 */
typedef struct {
    PyObject_HEAD
    // We will store the properties as native C types.
    int bits;
    double eps;
    double max;
    double min;
    int precision;
    double resolution;
} AtomFInfoObject;

/*
 * This defines the Python attributes for our `finfo` object. We use
 * PyMemberDef for direct, fast access to the C struct members.
 */
static PyMemberDef atom_finfo_members[] = {
    {"bits", T_INT, offsetof(AtomFInfoObject, bits), READONLY, "number of bits in the type"},
    {"eps", T_DOUBLE, offsetof(AtomFInfoObject, eps), READONLY, "machine epsilon"},
    {"max", T_DOUBLE, offsetof(AtomFInfoObject, max), READONLY, "largest representable number"},
    {"min", T_DOUBLE, offsetof(AtomFInfoObject, min), READONLY, "smallest positive normalized number"},
    {"precision", T_INT, offsetof(AtomFInfoObject, precision), READONLY, "approximate number of decimal digits of precision"},
    {"resolution", T_DOUBLE, offsetof(AtomFInfoObject, resolution), READONLY, "approximate decimal resolution"},
    {NULL} // Sentinel
};

/*
 * This is the master definition of our new `atom.finfo` Python type.
 */
static PyTypeObject AtomFInfo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "atom.finfo",
    .tp_basicsize = sizeof(AtomFInfoObject),
    .tp_itemsize = 0,
    // We will implement a custom __repr__ for this later if needed.
    .tp_members = atom_finfo_members,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "finfo(dtype) -> finfo object\n\nObject containing floating point type information.",
};

// --- NEW: Implementation of the atom.finfo() function ---

/*
 * This is the C function that gets executed when a user calls `atom.finfo()`.
 */
static PyObject*
atom_module_finfo(PyObject *self, PyObject *args)
{
    PyObject* dtype_arg;

    // Parse the single argument, which should be a dtype.
    if (!PyArg_ParseTuple(args, "O", &dtype_arg)) {
        return NULL;
    }

    // Check if the argument is one of our dtype objects.
    if (!PyObject_TypeCheck(dtype_arg, &AtomDType_Type)) {
        PyErr_SetString(PyExc_TypeError, "argument must be an atom.dtype");
        return NULL;
    }

    AtomDTypeObject *dtype_obj = (AtomDTypeObject *)dtype_arg;
    const Atom_DType* dtype_c = dtype_obj->dtype_c;

    // --- Input Validation ---
    // Check the 'kind' of the dtype. If it's not 'f' (float) or 'c' (complex) or 'V' (bfloat16),
    // raise a helpful error.
    if (dtype_c->kind != 'f' && dtype_c->kind != 'c' && dtype_c->kind != 'V') {
        PyErr_SetString(PyExc_TypeError,
            "finfo is only available for floating point and complex dtypes, try iinfo for integers.");
        return NULL;
    }

    // --- Create and Populate the finfo Object ---
    AtomFInfoObject *finfo_obj = (AtomFInfoObject *)AtomFInfo_Type.tp_alloc(&AtomFInfo_Type, 0);
    if (finfo_obj == NULL) {
        return NULL;
    }

    // Populate the finfo object based on the specific dtype.
    switch (dtype_c->type_id) {
        case ATOM_FLOAT32:
            finfo_obj->bits = 32;
            finfo_obj->eps = FLT_EPSILON;
            finfo_obj->max = FLT_MAX;
            finfo_obj->min = -FLT_MAX;
            finfo_obj->precision = 6;
            finfo_obj->resolution = 1e-6;
            break;
        case ATOM_FLOAT64:
            finfo_obj->bits = 64;
            finfo_obj->eps = DBL_EPSILON;
            finfo_obj->max = DBL_MAX;
            finfo_obj->min = -DBL_MAX;
            finfo_obj->precision = 15;
            finfo_obj->resolution = 1e-15;
            break;
        case ATOM_BFLOAT16: // Our custom type
            finfo_obj->bits = 16;
            finfo_obj->eps = 0.0078125; // 2**-7
            finfo_obj->max = 3.389531e+38; // From bfloat16 spec
            finfo_obj->min = -3.389531e+38;
            finfo_obj->precision = 2;
            finfo_obj->resolution = 1e-2;
            break;
        // For complex types, finfo returns info about the component float type.
        case ATOM_CFLOAT64:
            finfo_obj->bits = 32;
            finfo_obj->eps = FLT_EPSILON;
            finfo_obj->max = FLT_MAX;
            finfo_obj->min = FLT_MIN;
            finfo_obj->precision = 6;
            finfo_obj->resolution = 1e-6;
            break;
        case ATOM_CFLOAT128:
            finfo_obj->bits = 64;
            finfo_obj->eps = DBL_EPSILON;
            finfo_obj->max = DBL_MAX;
            finfo_obj->min = DBL_MIN;
            finfo_obj->precision = 15;
            finfo_obj->resolution = 1e-15;
            break;
        // Add other float types like LONGDOUBLE here if needed.
        default:
            // This case should not be reached due to the 'kind' check above,
            // but it is good practice to have a default.
            Py_DECREF(finfo_obj);
            PyErr_SetString(PyExc_TypeError, "finfo not available for this type.");
            return NULL;
    }

    return (PyObject *)finfo_obj;
}

// =====================================================================================
//  4. DEFINE THE `atom` MODULE AND ITS INITIALIZATION
// =====================================================================================

// --- NEW: Module-level functions ---

/*
 * This array defines the functions available at the module level,
 * e.g., `atom.finfo()`.
 */
static PyMethodDef atom_module_methods[] = {
    {
        "finfo", // The name of the function in Python
        atom_module_finfo, // A pointer to the C implementation
        METH_VARARGS, // Indicates it takes positional arguments (a tuple)
        "finfo(dtype) -> finfo object\n\nGet information about a floating point data type." // Docstring
    },
    {NULL, NULL, 0, NULL} // Sentinel
};

/*
 * This struct provides the metadata for the Python module itself.
 */
static struct PyModuleDef atom_module_definition = {
    PyModuleDef_HEAD_INIT,
    "atom",                                                        // The name of the module.
    "A library of fundamental data types built from scratch in C.",// The module's docstring.
    -1,                                                            // -1 means the module keeps no global state.
    atom_module_methods,
};

/*
 * This is the official entry point for the module. When a user in Python
 * executes `import atom`, the Python interpreter searches for and calls this
 * specific function. Its name *must* be `PyInit_<module_name>`.
 *
 * Its job is to build and return a fully initialized module object.
 */
PyMODINIT_FUNC PyInit_atom(void) {
    PyObject *module;

    // This is a mandatory step. It takes our static `PyTypeObject` definition,
    // fills in any missing details (like inheriting from `object`), and makes
    // it ready for use.
    if (PyType_Ready(&AtomDType_Type) < 0) {
        return NULL; // This would be a catastrophic failure.
    }

    // --- NEW: Finalize the `atom.finfo` type ---
    if (PyType_Ready(&AtomFInfo_Type) < 0) {
        return NULL;
    }

    // `PyModule_Create` takes our module definition and creates the module object.
    module = PyModule_Create(&atom_module_definition);
    if (module == NULL) {
        Py_DECREF(&AtomDType_Type); // Clean up the type if module creation fails.
        return NULL;
    }

    // --- Add the `dtype` type object itself to the module ---
    // This allows users to call `atom.dtype(...)`.
    Py_INCREF(&AtomDType_Type); // We must increment the ref count before giving it to the module.
    if (PyModule_AddObject(module, "dtype", (PyObject *)&AtomDType_Type) < 0) {
        // If `PyModule_AddObject` fails, it does NOT steal the reference, so we must DECREF.
        Py_DECREF(&AtomDType_Type);
        Py_DECREF(module);
        return NULL;
    }

    // --- Loop through all our C types and add pre-made instances to the module ---
    for (Atom_DTypeID id = 0; id < ATOM_NTYPES; ++id) {
        const Atom_DType* dtype_c = get_atom_dtype(id);

        if (dtype_c != NULL) { // If the C blueprint exists...
            if (add_dtype_to_module(module, dtype_c) < 0) { // ...add it to the module.
                // If adding any single type fails, we must abort initialization.
                Py_DECREF(module);
                Py_DECREF(&AtomDType_Type);
                PyErr_SetString(PyExc_SystemError, "Failed to add a dtype to the atom module.");
                return NULL;
            }
        }
    }

    // If we get here, the module is fully built and populated.
    return module;
}