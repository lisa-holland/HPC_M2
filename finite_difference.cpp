// finite_difference.cpp
//
// Demonstrates the order of accuracy of finite-difference approximations
// of the first derivative:
//
//   Forward difference  (1st order):  f'(x) ~= [f(x+h) - f(x)]     / h
//   Backward difference (1st order):  f'(x) ~= [f(x)   - f(x-h)]   / h
//   Central difference  (2nd order):  f'(x) ~= [f(x+h) - f(x-h)]   / (2h)
//
// For a range of geometrically-decreasing step sizes h, the absolute error
// between each approximation and the exact derivative is computed and
// written to an HDF5 file. A Jupyter notebook (convergence_analysis.ipynb)
// then reads that file and shows, via log-log plots and least-squares
// slope fits, that the forward/backward schemes converge as O(h) while
// the central scheme converges as O(h^2).
//
// Build (requires libhdf5-dev / the HDF5 C++ API):
//   h5c++ -O2 -std=c++17 -o fd_convergence finite_difference.cpp
//   ./fd_convergence
//
// or with CMake (see CMakeLists.txt):
//   cmake -B build && cmake --build build && ./build/fd_convergence
//
// Output: fd_results.h5

#include <H5Cpp.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------
// A single test case: a function, its exact derivative, and the point at
// which the derivative is approximated.
// -----------------------------------------------------------------------
struct TestCase {
    std::string group_name;    // HDF5 group name for this case
    std::string function_name; // human-readable label
    double x0;                 // evaluation point
    std::function<double(double)> f;
    std::function<double(double)> f_exact_derivative;
};

// -----------------------------------------------------------------------
// Results of sweeping h for one test case.
// -----------------------------------------------------------------------
struct SweepResult {
    std::vector<double> h;
    std::vector<double> error_forward;
    std::vector<double> error_backward;
    std::vector<double> error_central;
    double exact_derivative;
};

// Forward difference: O(h)
double forward_difference(const std::function<double(double)>& f, double x, double h) {
    return (f(x + h) - f(x)) / h;
}

// Backward difference: O(h)
double backward_difference(const std::function<double(double)>& f, double x, double h) {
    return (f(x) - f(x - h)) / h;
}

// Central difference: O(h^2)
double central_difference(const std::function<double(double)>& f, double x, double h) {
    return (f(x + h) - f(x - h)) / (2.0 * h);
}

// -----------------------------------------------------------------------
// Sweep h = h0 * r^n for n = 0 .. n_steps-1, evaluating all three schemes
// and recording the absolute error against the exact derivative.
// -----------------------------------------------------------------------
SweepResult run_sweep(const TestCase& tc, double h0, double r, int n_steps) {
    SweepResult result;
    result.h.reserve(n_steps);
    result.error_forward.reserve(n_steps);
    result.error_backward.reserve(n_steps);
    result.error_central.reserve(n_steps);

    const double exact = tc.f_exact_derivative(tc.x0);
    result.exact_derivative = exact;

    double h = h0;
    for (int n = 0; n < n_steps; ++n) {
        const double fwd = forward_difference(tc.f, tc.x0, h);
        const double bwd = backward_difference(tc.f, tc.x0, h);
        const double ctr = central_difference(tc.f, tc.x0, h);

        result.h.push_back(h);
        result.error_forward.push_back(std::abs(fwd - exact));
        result.error_backward.push_back(std::abs(bwd - exact));
        result.error_central.push_back(std::abs(ctr - exact));

        h *= r; // r < 1, so h shrinks geometrically each step
    }
    return result;
}

// -----------------------------------------------------------------------
// Write a std::vector<double> as a 1-D HDF5 dataset under the given group.
// -----------------------------------------------------------------------
void write_dataset(H5::Group& group, const std::string& name, const std::vector<double>& data) {
    const hsize_t dims[1] = {data.size()};
    H5::DataSpace dataspace(1, dims);
    H5::DataSet dataset = group.createDataSet(name, H5::PredType::NATIVE_DOUBLE, dataspace);
    dataset.write(data.data(), H5::PredType::NATIVE_DOUBLE);
}

// Write a scalar double as an HDF5 attribute on a group.
void write_scalar_attr(H5::Group& group, const std::string& name, double value) {
    H5::DataSpace scalar_space(H5S_SCALAR);
    H5::Attribute attr = group.createAttribute(name, H5::PredType::NATIVE_DOUBLE, scalar_space);
    attr.write(H5::PredType::NATIVE_DOUBLE, &value);
}

// Write a string as an HDF5 attribute on a group.
void write_string_attr(H5::Group& group, const std::string& name, const std::string& value) {
    H5::StrType str_type(H5::PredType::C_S1, H5T_VARIABLE);
    H5::DataSpace scalar_space(H5S_SCALAR);
    H5::Attribute attr = group.createAttribute(name, str_type, scalar_space);
    attr.write(str_type, value);
}

int main() {
    // ---------------------------------------------------------------
    // Define the test cases. Two different functions/points are used
    // so the convergence behavior is shown to be independent of the
    // particular choice of function.
    // ---------------------------------------------------------------
    std::vector<TestCase> cases;

    cases.push_back(TestCase{
        "sin", "sin(x)", 0.8,
        [](double x) { return std::sin(x); },
        [](double x) { return std::cos(x); }
    });

    cases.push_back(TestCase{
        "exp", "exp(x)", 0.5,
        [](double x) { return std::exp(x); },
        [](double x) { return std::exp(x); }
    });

    cases.push_back(TestCase{
        "cubic", "x^3 - 2x", 1.3,
        [](double x) { return x * x * x - 2.0 * x; },
        [](double x) { return 3.0 * x * x - 2.0; }
    });

    // Step-size sweep parameters: h0 = 0.1, halved 30 times.
    // This range deliberately extends down to h ~ 1e-10, small enough
    // that floating-point round-off error eventually dominates and the
    // error curves turn upward -- an instructive feature in its own
    // right that the notebook also highlights.
    const double h0 = 0.1;
    const double r = 0.5;
    const int n_steps = 31;

    // ---------------------------------------------------------------
    // Create the HDF5 file and one group per test case.
    // ---------------------------------------------------------------
    const std::string filename = "fd_results.h5";
    H5::H5File file(filename, H5F_ACC_TRUNC);

    for (const auto& tc : cases) {
        std::cout << "Running sweep for " << tc.function_name
                  << " at x0 = " << tc.x0 << " ...\n";

        SweepResult res = run_sweep(tc, h0, r, n_steps);

        H5::Group group = file.createGroup("/" + tc.group_name);

        write_string_attr(group, "function_name", tc.function_name);
        write_scalar_attr(group, "x0", tc.x0);
        write_scalar_attr(group, "exact_derivative", res.exact_derivative);

        write_dataset(group, "h", res.h);
        write_dataset(group, "error_forward", res.error_forward);
        write_dataset(group, "error_backward", res.error_backward);
        write_dataset(group, "error_central", res.error_central);
    }

    // Top-level metadata describing the schemes and their expected order.
    H5::Group meta = file.createGroup("/metadata");
    write_string_attr(meta, "forward_scheme", "f'(x) ~= [f(x+h) - f(x)] / h  (expected order 1)");
    write_string_attr(meta, "backward_scheme", "f'(x) ~= [f(x) - f(x-h)] / h  (expected order 1)");
    write_string_attr(meta, "central_scheme", "f'(x) ~= [f(x+h) - f(x-h)] / (2h)  (expected order 2)");
    write_scalar_attr(meta, "h0", h0);
    write_scalar_attr(meta, "ratio", r);

    file.close();

    std::cout << "Wrote results for " << cases.size() << " test case(s) to "
              << filename << "\n";
    return 0;
}
