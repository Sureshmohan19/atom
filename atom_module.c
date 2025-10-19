// atom_module.c
// The C-API bridge to expose the Atom library to Python.

// This macro is required for the modern Python C-API.
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h" // Required for PyMemberDef

// We must include our own library's header to access our types.
#include "atom_types.h"

// --- 1. The Python `atom.dtype` Object Definition ---

// This is the C struct that will represent our dtype object in Python.
// It MUST start with PyObject_HEAD.
typedef struct {
    PyObject_HEAD
    // This will hold a pointer to our pure C Atom_DType struct.
    const Atom_DType* dtype_c;
} AtomDTypeObject;


// --- 2. The Python `atom.dtype` Type Definition ---

// This is the massive static struct that describes our new type to Python.
// It tells Python the type's name, size, and what functions to call for
// basic operations like printing or getting attributes.
static PyTypeObject AtomDType_Type = {
    // Basic object setup
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "atom.dtype",              // The name of the type in Python
    .tp_basicsize = sizeof(AtomDTypeObject), // The size of our C struct
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "An Atom data type object.",
};


// --- 3. The Python Module Definition ---

// This struct defines the module itself. For now, it's simple: it just has a name.
static struct PyModuleDef atom_module = {
    PyModuleDef_HEAD_INIT,
    "atom", // Module name in Python
    "A library of fundamental data types built from scratch in C.", // Docstring
    -1,     // No per-interpreter state
    NULL,   // No functions in the module... yet
};


// --- 4. The Module Initialization Function ---

// This is the single most important function. Python calls this when you run `import atom`.
// Its job is to create the module and populate it with our types.
PyMODINIT_FUNC PyInit_atom(void) {
    PyObject *module;

    // -- Finalize our custom `atom.dtype` type --
    // This call is crucial. It prepares our PyTypeObject for use.
    if (PyType_Ready(&AtomDType_Type) < 0) {
        return NULL; // Failed to prepare the type
    }

    // -- Create the module object --
    module = PyModule_Create(&atom_module);
    if (module == NULL) {
        Py_DECREF(&AtomDType_Type); // Clean up the type on failure
        return NULL;
    }

    // --- Create and add our int8 dtype to the module ---

    // 1. Get the C blueprint using our C-API function.
    const Atom_DType* int8_c_type = get_atom_dtype(ATOM_INT8);
    if (int8_c_type == NULL) {
        // This should never happen if our C library is correct.
        Py_DECREF(module);
        Py_DECREF(&AtomDType_Type);
        PyErr_SetString(PyExc_SystemError, "Failed to get ATOM_INT8 C dtype.");
        return NULL;
    }

    // 2. Create an instance of our new Python type (`atom.dtype`).
    AtomDTypeObject *int8_py_obj = (AtomDTypeObject *)AtomDType_Type.tp_alloc(&AtomDType_Type, 0);
    if (int8_py_obj == NULL) {
        Py_DECREF(module);
        Py_DECREF(&AtomDType_Type);
        return NULL;
    }

    // 3. Link the Python object to our C blueprint.
    int8_py_obj->dtype_c = int8_c_type;

    // 4. Add the new Python object to the module with the name "int8".
    // PyModule_AddObject returns -1 on error and steals a reference to int8_py_obj on success.
    if (PyModule_AddObject(module, "int8", (PyObject *)int8_py_obj) < 0) {
        Py_DECREF(int8_py_obj); // Must decref on failure
        Py_DECREF(module);
        Py_DECREF(&AtomDType_Type);
        return NULL;
    }

    // -- Success! Return the initialized module. --
    return module;
}