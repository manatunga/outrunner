/// @file test_matmul.cpp
/// @brief Correctness tests for Outrunner's CPU matrix multiplication.
///
/// Tests the sequential CPU matrix multiplication implementation
/// across rectangular, square, single-element, and zero-dimension
/// matrix configurations.
///
/// Verifies numerical correctness, row-major indexing, and
/// expected behaviour when matrix dimensions are zero.

#include <cstddef>
#include <iostream>

#include "outrunner/cpu/matmul.hpp"


/// @brief Compares two arrays element by element.
///
/// Reports the first mismatched element, including its index,
/// expected value, and actual value.
///
/// @param actual The computed output array.
/// @param expected The expected output array.
/// @param count Number of elements to compare.
///
/// @return true if every element matches; otherwise false.
bool check_results(
    const float* actual,
    const float* expected,
    std::size_t count
) {
    for (std::size_t i = 0; i < count; i++) {
        if (actual[i] != expected[i]) {
            std::cerr << "FAIL: index " << i
                      << " — expected " << expected[i]
                      << ", got " << actual[i] << std::endl;
            return false;
        }
    }

    return true;
}


/// @brief Tests multiplication of two rectangular matrices.
///
/// Multiplies a 2×3 matrix by a 3×2 matrix.
/// Verifies the resulting 2×2 matrix.
bool test_rectangular_matrices() {
    float a[6] = {1, 2, 3, 4, 5, 6};
    float b[6] = {7, 8, 9, 10, 11, 12};

    float expected[4] = {58, 64, 139, 154};
    float actual[4];

    outrunner::cpu::matmul(a, b, actual, 2, 3, 2);

    return check_results(actual, expected, 4);
}


/// @brief Tests multiplication of two square matrices.
///
/// Multiplies two 2×2 matrices and verifies the resulting
/// 2×2 matrix.
bool test_square_matrices() {
    float a[4] = {1, 2, 3, 4};
    float b[4] = {5, 6, 7, 8};

    float expected[4] = {19, 22, 43, 50};
    float actual[4];

    outrunner::cpu::matmul(a, b, actual, 2, 2, 2);

    return check_results(actual, expected, 4);
}


/// @brief Tests multiplication of two single-element matrices.
///
/// Verifies the smallest non-empty matrix multiplication operation.
bool test_single_element() {
    float a[1] = {3.0f};
    float b[1] = {4.0f};

    float expected[1] = {12.0f};
    float actual[1];

    outrunner::cpu::matmul(a, b, actual, 1, 1, 1);

    return check_results(actual, expected, 1);
}


/// @brief Tests multiplication when the shared dimension is zero.
///
/// Verifies that every output element is initialized to zero
/// without accessing either input matrix.
bool test_zero_shared_dimension() {
    float expected[6] = {0, 0, 0, 0, 0, 0};
    float actual[6];

    outrunner::cpu::matmul(nullptr, nullptr, actual, 2, 0, 3);

    return check_results(actual, expected, 6);
}


/// @brief Tests multiplication when the output has zero rows.
///
/// Verifies that no input or output elements are accessed
/// when the first matrix has no rows.
bool test_zero_output_rows() {
    float b[6] = {1, 2, 3, 4, 5, 6};

    outrunner::cpu::matmul(nullptr, b, nullptr, 0, 2, 3);

    return true;
}


/// @brief Tests multiplication when the output has zero columns.
///
/// Verifies that no elements of the second input or output
/// matrix are accessed when the second matrix has no columns.
bool test_zero_output_columns() {
    float a[4] = {1, 2, 3, 4};

    outrunner::cpu::matmul(a, nullptr, nullptr, 2, 2, 0);

    return true;
}


/// @brief Runs all matrix multiplication tests.
///
/// @return 0 if every test passes; otherwise 1.
int main() {
    bool all_passed = true;

    std::cout << "Attempting test_rectangular_matrices ...\n";
    if (!test_rectangular_matrices()) {
        all_passed = false;
    }

    std::cout << "Attempting test_square_matrices ...\n";
    if (!test_square_matrices()) {
        all_passed = false;
    }

    std::cout << "Attempting test_single_element ...\n";
    if (!test_single_element()) {
        all_passed = false;
    }

    std::cout << "Attempting test_zero_shared_dimension ...\n";
    if (!test_zero_shared_dimension()) {
        all_passed = false;
    }

    std::cout << "Attempting test_zero_output_rows ...\n";
    if (!test_zero_output_rows()) {
        all_passed = false;
    }

    std::cout << "Attempting test_zero_output_columns ...\n";
    if (!test_zero_output_columns()) {
        all_passed = false;
    }

    if (all_passed) {
        std::cout << "PASS: All tests have passed.\n";
        return 0;
    }

    std::cerr << "FAIL: One or more tests failed.\n";
    return 1;
}
