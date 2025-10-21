# setup.py

from setuptools import setup, Extension

# This defines the C extension module we want to build.
atom_module = Extension(
    # 1. The name of the module in Python, e.g., `import atom`.
    'atom',

    # 2. The list of C source files to compile.
    #    We need our new "bindings" file and our existing "types" file.
    sources=['atom_module.c', 'atom_types.c', 'atom_conversions.c', 'dtype_object.c'],

    # 3. (Optional) List of directories to search for header files.
    #    Since our .h is in the same directory, we don't need this yet.
    # include_dirs=[],
)

# This is the main function that tells setuptools how to build our project.
setup(
    # The name of your package as it would appear on PyPI.
    name='atom-types',
    version='0.0.1',
    description='A library of fundamental data types built from scratch in C.',

    # This tells setup to build the C extension we defined above.
    ext_modules=[atom_module],
)