/// @file vector_add.hpp
/// @brief Public interface for Outrunner's CPU vector-addition operation.
///
/// Declares the sequential reference operation used to validate
/// numerical correctness during development of the compute runtime.
#pragma once

#include <cstddef>

namespace outrunner::cpu {

    /// @brief Performs element-wise addition of two floating-point arrays.
    ///
    /// Computes the sum of corresponding elements from two input arrays
    /// and writes the results into a caller-provided output array.
    ///
    /// For each index i in the range [0, count):
    ///
    ///     output[i] = a[i] + b[i]
    ///
    /// @param a Pointer to the first input array.
    /// @param b Pointer to the second input array.
    /// @param output Pointer to the destination array.
    /// @param count Number of floating-point elements to process.
    ///
    /// @pre When count is greater than zero, all three pointers must
    ///     reference valid arrays containing at least count accessible
    ///     float elements.
    /// @pre The output array must not overlap either input array.
    ///
    /// @post The output array contains the element-wise sums.
    /// @post The input arrays remain unchanged.
    ///
    /// @note This function is part of Outrunner's sequential CPU
    ///     reference implementation.
    /// @note Memory allocation and deallocation are the caller's
    ///     responsibility.
    /// @note A count of zero performs no element operations and does
    ///     not require the pointers to reference accessible elements.
    void add_vector(
        const float* a,
        const float* b,
        float* output,
        std::size_t count
    );

} // namespace outrunner::cpu
