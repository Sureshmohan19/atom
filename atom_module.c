// The C-API bridge to expose the Atom library to Python.

// This macro is required for the modern Python C-API.
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

// ATOM C-library's header to access our types.
#include "atom_types.h"

// --- 1. The Python `atom.dtype` Object Definition ---

// This is the C struct that will represent our dtype object in Python.
// It MUST start with PyObject_HEAD.
typedef struct {
    PyObject_HEAD
    // This will hold a pointer to our pure C Atom_DType struct.
    const Atom_DType* dtype_c;
} AtomDTypeObject;

// --- NEW: FORWARD DECLARATION ---
// Add this line. It tells the compiler that AtomDType_Type exists
// before it is fully defined.
static PyTypeObject AtomDType_Type;

// --- 1.5. NEW: The Representation Function for atom.dtype ---

// This function is called when Python needs a string representation of our object.
// It corresponds to the __repr__ method in Python.
static PyObject*
atom_dtype_str(AtomDTypeObject *self)
{
    // self->dtype_c points to our pure C struct.
    // We can access its 'name' field directly.
    if (self->dtype_c == NULL) {
        return PyUnicode_FromString("<atom.dtype NULL>");
    }

    // PyUnicode_FromFormat is a C-API function like printf,
    // but it creates a Python string object.
    // Create a Python string from our C string 'name'.
    return PyUnicode_FromString(self->dtype_c->name);
}

// --- 1.7. NEW: Getter/Setter Definitions for atom.dtype ---

// Getter function for the 'name' attribute.
static PyObject*
atom_dtype_get_name(AtomDTypeObject *self, void *closure) {
    return PyUnicode_FromString(self->dtype_c->name);
}

// Getter function for the 'elsize' attribute.
static PyObject*
atom_dtype_get_elsize(AtomDTypeObject *self, void *closure) {
    return PyLong_FromSize_t(self->dtype_c->elsize);
}

// Getter function for the 'alignment' attribute.
static PyObject*
atom_dtype_get_alignment(AtomDTypeObject *self, void *closure) {
    return PyLong_FromSize_t(self->dtype_c->alignment);
}

// Getter function for 'kind', 'type_char', and 'byteorder' attributes.
static PyObject*
atom_dtype_get_char_attr(const char* attr_name, char attr_val) {
    // This is a small helper to avoid repetitive code.
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

/*
 * This array maps a Python attribute name to a C "getter" function.
 * Since we don't want to allow setting these attributes, the 'setter' is NULL.
 */
static PyGetSetDef atom_dtype_getsetters[] = {
    {
        "name", // Attribute name in Python
        (getter)atom_dtype_get_name, // The C function to call
        NULL, // No setter function
        "The common name of the data type (e.g., 'int32')", // Docstring
        NULL // Closure data (not needed)
    },
    {"itemsize", (getter)atom_dtype_get_elsize, NULL, "The size of the data type in bytes", NULL},
    {"alignment", (getter)atom_dtype_get_alignment, NULL, "The required memory alignment", NULL},
    {"kind", (getter)atom_dtype_get_kind, NULL, "A character for the general kind of the type", NULL},
    {"type", (getter)atom_dtype_get_type_char, NULL, "A single character code for the type", NULL},
    {"char", (getter)atom_dtype_get_type_char, NULL, "A single character code for the type (alias for 'type')", NULL},
    {"byteorder", (getter)atom_dtype_get_byteorder, NULL, "A character indicating the byte order", NULL},

    // Sentinel marks the end of the array.
    {NULL}
};

/*
 * This function is called when a user creates a new dtype instance from Python,
 * e.g., `atom.dtype('int32')`. It is the implementation of __new__.
 */
static PyObject*
atom_dtype_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
    // We will only accept one positional argument, a string, for now.
    // The "O" format code gets a single Python object.
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL; // PyArg_ParseTuple sets the error.
    }

    // Check if the passed object is a Python string.
    if (!PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "dtype constructor expects a string");
        return NULL;
    }

    // Convert the Python string to a C string.
    const char* dtype_name = PyUnicode_AsUTF8(obj);
    if (dtype_name == NULL) {
        // PyUnicode_AsUTF8 sets an error on failure.
        return NULL;
    }

    // --- Find the matching C blueprint in our library ---
    const Atom_DType* found_dtype_c = NULL;
    for (Atom_DTypeID id = 0; id < ATOM_NTYPES; ++id) {
        const Atom_DType* dtype_c = get_atom_dtype(id);
        if (dtype_c != NULL && strcmp(dtype_c->name, dtype_name) == 0) {
            found_dtype_c = dtype_c;
            break; // Found it!
        }
    }

    // If we didn't find a match, raise an error.
    if (found_dtype_c == NULL) {
        PyErr_Format(PyExc_TypeError, "'%s' is not a valid Atom data type", dtype_name);
        return NULL;
    }

    // --- Create and return the new Python object ---

    // Allocate a new Python object of our custom type.
    AtomDTypeObject *self = (AtomDTypeObject *)subtype->tp_alloc(subtype, 0);
    if (self == NULL) {
        return NULL;
    }

    // Link our new Python object to the C blueprint we found.
    self->dtype_c = found_dtype_c;

    return (PyObject *)self;
}

/*
 * This function is called when two atom.dtype objects are compared,
 * e.g., `atom.int32 == atom.dtype('int32')`.
 */
static PyObject*
atom_dtype_richcompare(AtomDTypeObject *self, PyObject *other, int op)
{
    // First, check if the 'other' object is also an atom.dtype.
    // PyObject_TypeCheck is a safe way to check an object's type.
    if (!PyObject_TypeCheck(other, &AtomDType_Type)) {
        // If it's not our type, we don't know how to compare it.
        // Returning Py_NotImplemented allows Python to try other comparison methods.
        Py_RETURN_NOTIMPLEMENTED;
    }

    // Cast the other object to our type so we can access its C struct.
    AtomDTypeObject *other_dtype = (AtomDTypeObject *)other;

    // Get the underlying C pointers for both objects.
    const Atom_DType* self_c = self->dtype_c;
    const Atom_DType* other_c = other_dtype->dtype_c;

    // Check for NULL pointers, just in case.
    if (self_c == NULL || other_c == NULL) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    // The core comparison logic: two dtypes are equal if their C pointers are the same.
    // Since we have one canonical C struct for each type, this is a fast and
    // reliable way to check for equality of meaning.
    int are_equal = (self_c == other_c);

    // Now, handle the specific comparison operation requested by Python.
    switch (op) {
        case Py_EQ: // For `==`
            if (are_equal) Py_RETURN_TRUE;
            else Py_RETURN_FALSE;
        case Py_NE: // For `!=`
            if (!are_equal) Py_RETURN_TRUE;
            else Py_RETURN_FALSE;
        default:
            // For any other comparison like <, >, etc., we don't support them.
            Py_RETURN_NOTIMPLEMENTED;
    }
}

// --- 2. The Python `atom.dtype` Type Definition ---

// This is the massive static struct that describes our new type to Python.
// It tells Python the type's name, size, and what functions to call for
// basic operations like printing or getting attributes.
static PyTypeObject AtomDType_Type = {
    
    // Basic object setup
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "atom.dtype",              
    .tp_basicsize = sizeof(AtomDTypeObject), 
    .tp_itemsize = 0,
    .tp_repr = (reprfunc)atom_dtype_str,
    .tp_str  = (reprfunc)atom_dtype_str, 
    
    // Use the get/set slot to link our attribute functions.
    .tp_getset = atom_dtype_getsetters,

    // Add this line to set our constructor function.
    .tp_new = atom_dtype_new,

    // Add this line to set our comparison function.
    .tp_richcompare = (richcmpfunc)atom_dtype_richcompare,


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

/*
 * Helper function: Creates a Python AtomDTypeObject from a C Atom_DType blueprint
 * and adds it to the given module.
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

    // Create the module object.
    static struct PyModuleDef atom_module = {
        PyModuleDef_HEAD_INIT,
        "atom",
        "A library of fundamental data types built from scratch in C.",
        -1,
        NULL,
    };
    module = PyModule_Create(&atom_module);
    if (module == NULL) {
        Py_DECREF(&AtomDType_Type);
        return NULL;
    }

    // --- Add the `dtype` type object to the module ---
    Py_INCREF(&AtomDType_Type); // We must increment the reference count before adding.
    if (PyModule_AddObject(module, "dtype", (PyObject *)&AtomDType_Type) < 0) {
        Py_DECREF(&AtomDType_Type);
        Py_DECREF(module);
        return NULL;
    }

    // --- Loop through all our C types and add them to the Python module ---
    for (Atom_DTypeID id = 0; id < ATOM_NTYPES; ++id) {
        // Use our C-API to get the blueprint for the current type ID.
        const Atom_DType* dtype_c = get_atom_dtype(id);

        // If the C blueprint exists (i.e., it's not a deferred type),
        // then add it to our Python module.
        if (dtype_c != NULL) {
            if (add_dtype_to_module(module, dtype_c) < 0) {
                // If adding any single type fails, we must abort the entire
                // module initialization.
                Py_DECREF(module);
                Py_DECREF(&AtomDType_Type);
                PyErr_SetString(PyExc_SystemError, "Failed to add a dtype to the atom module.");
                return NULL;
            }
        }
    }

    // Success! Return the fully populated module.
    return module;
}