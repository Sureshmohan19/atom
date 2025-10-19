# Atom

**Atom** is a lightweight, high-performance library for fundamental numeric data types, built from scratch in C and exposed to Python.

## Vision

The goal of Atom is to provide a clean, modern, and minimal implementation of the core data types that underpin scientific computing. By focusing on a clear C foundation and a direct Python C-API interface, this project serves as both a practical tool and an educational exploration into the architecture of numerical libraries.

While the initial focus is on defining a robust type system, the long-term vision is to build a comprehensive numerical computing library, including:

-   A custom N-dimensional array object.
-   A library of optimized mathematical functions (ufuncs).
-   Support for specialized, non-native data types for machine learning.

## Core Features (Current)

-   A pure C library defining a complete set of native numeric data types (`int8`, `float32`, `complex128`, etc.).
-   A Python extension module `atom` that exposes these C types as first-class Python objects.
-   A familiar `atom.dtype` constructor for creating type objects from strings.
-   Direct access to type properties like `.itemsize`, `.kind`, and `.name`.

## Installation

Currently, the library must be built from source. Ensure you have a C compiler and Python development headers installed.

```bash
# Clone the repository
git clone https://github.com/Sureshmohan19/atom.git
cd atom

# Create a virtual environment and activate it
python3 -m venv .venv
source .venv/bin/activate

# Build and install the package
pip install .