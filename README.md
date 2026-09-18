# Finite-difference order-of-accuracy demo

Demonstrates numerically that:

- **forward** difference `f'(x) ≈ [f(x+h) − f(x)] / h` is **1st-order** accurate
- **backward** difference `f'(x) ≈ [f(x) − f(x−h)] / h` is **1st-order** accurate
- **central** difference `f'(x) ≈ [f(x+h) − f(x−h)] / (2h)` is **2nd-order** accurate

for three test functions (`sin(x)`, `exp(x)`, `x^3 − 2x`), by sweeping the step
size `h` over 31 geometrically-spaced values (`h0 = 0.1`, halved each step)
and comparing each scheme's error against the exact derivative.

## Files

| File                         | Purpose                                                      |
|-------------------------------|---------------------------------------------------------------|
| `finite_difference.cpp`       | C++ program; writes results to `fd_results.h5`                |
| `CMakeLists.txt`               | CMake build (uses `find_package(HDF5)`)                       |
| `Makefile`                     | Simple build using the `h5c++` wrapper compiler                |
| `convergence_analysis.ipynb`   | Jupyter notebook: loads `fd_results.h5`, plots log-log convergence, fits observed order |

## Requirements

- A C++17 compiler
- **HDF5 with the C++ API** (`libhdf5-dev` on Debian/Ubuntu: `sudo apt install libhdf5-dev`;
  `brew install hdf5` on macOS — make sure `h5c++` ends up on your `PATH`)
- Python 3 with `h5py`, `numpy`, `matplotlib` (and optionally `pandas`) for the notebook:
  `pip install h5py numpy matplotlib pandas jupyter`

## Build and run

Using the Makefile (simplest — relies on the `h5c++` wrapper compiler):

```bash
make
./fd_convergence
```

Or using CMake:

```bash
cmake -B build
cmake --build build
./build/fd_convergence
```

Either way this produces `fd_results.h5` in the current directory, containing
one HDF5 group per test function (`/sin`, `/exp`, `/cubic`), each with:

- attributes `function_name`, `x0`, `exact_derivative`
- datasets `h`, `error_forward`, `error_backward`, `error_central`

and a `/metadata` group describing the three schemes.

## View the results

```bash
jupyter notebook convergence_analysis.ipynb
```

The notebook reproduces log-log error-vs-`h` plots for each function and
fits the slope of each curve, which comes out close to **1** for the
forward/backward schemes and close to **2** for the central scheme —
directly confirming their theoretical order of accuracy. It also shows how,
at very small `h`, floating-point round-off error eventually dominates and
the error starts increasing again.
