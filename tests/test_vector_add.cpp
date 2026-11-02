/// @file test_vector_add.cpp
/// @brief Correctness tests for Outrunner's CPU vector addition.
///
/// Tests the sequential CPU vector addition implementation across
/// ordinary, single-element, mixed-value, and zero-element inputs.
///
/// Verifies element-wise numerical correctness and ensures that
/// zero-element operations do not access the input or output buffers.

#include <cstddef>
#include <iostream>

#include "outrunner/cpu/vector_add.hpp"


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


/// @brief Tests addition of two four-element arrays.
///
/// Verifies that corresponding elements are added and written
/// to the correct output positions.
bool test_four_elements() {
    float a[4] = {1.5f, 2.0f, 3.5f, 4.0f};
    float b[4] = {0.5f, 3.0f, 1.5f, 6.0f};

    float expected[4] = {2.0f, 5.0f, 5.0f, 10.0f};
    float actual[4];

    outrunner::cpu::add_vector(a, b, actual, 4);

    return check_results(actual, expected, 4);
}


/// @brief Tests addition with a zero-element input.
///
/// Verifies that no input or output elements are accessed
/// when the requested element count is zero.
bool test_empty_array() {
    outrunner::cpu::add_vector(nullptr, nullptr, nullptr, 0);

    return true;
}


/// @brief Tests addition of two single-element arrays.
///
/// Verifies the smallest non-empty vector addition operation.
bool test_single_element() {
    float a[1] = {7.5f};
    float b[1] = {2.5f};

    float expected[1] = {10.0f};
    float actual[1];

    outrunner::cpu::add_vector(a, b, actual, 1);

    return check_results(actual, expected, 1);
}


/// @brief Tests addition using positive, negative, and zero values.
///
/// Verifies that element-wise addition works correctly
/// with mixed input values.
bool test_mixed_values() {
    float a[4] = {-2.0f, 0.0f, 4.5f, -1.5f};
    float b[4] = {3.0f, -2.0f, -1.5f, 1.5f};

    float expected[4] = {1.0f, -2.0f, 3.0f, 0.0f};
    float actual[4];

    outrunner::cpu::add_vector(a, b, actual, 4);

    return check_results(actual, expected, 4);
}


/// @brief Runs all vector addition tests.
///
/// @return 0 if every test passes; otherwise 1.
int main() {
    bool all_passed = true;

    std::cout << "Attempting test_four_elements ...\n";
    if (!test_four_elements()) {
        all_passed = false;
    }

    std::cout << "Attempting test_empty_array ...\n";
    if (!test_empty_array()) {
        all_passed = false;
    }

    std::cout << "Attempting test_single_element ...\n";
    if (!test_single_element()) {
        all_passed = false;
    }

    std::cout << "Attempting test_mixed_values ...\n";
    if (!test_mixed_values()) {
        all_passed = false;
    }

    if (all_passed) {
        std::cout << "PASS: All tests have passed.\n";
        return 0;
    }

    std::cerr << "FAIL: One or more tests failed.\n";
    return 1;
}
