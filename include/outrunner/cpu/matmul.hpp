/// @file matmul.hpp
/// @brief Public interface for Outrunner's CPU matrix multiplication.
///
/// Declares the sequential reference implementation of matrix
/// multiplication for single-precision floating-point matrices.
///
/// Matrices are represented as contiguous, row-major arrays.
/// The operation supports compatible rectangular matrix dimensions.
///
/// This interface provides a correctness baseline for validating
/// future CPU work-item execution and Vulkan GPU compute backends.
#pragma once

#include <cstddef>

namespace outrunner::cpu {

    /// @brief Multiplies two row-major floating-point matrices.
    ///
    /// Computes C = A × B, where:
    /// - A has dimensions m × k.
    /// - B has dimensions k × n.
    /// - C has dimensions m × n.
    ///
    /// @param a Pointer to the first input matrix.
    /// @param b Pointer to the second input matrix.
    /// @param output Pointer to the destination matrix.
    /// @param m Number of rows in A.
    /// @param k Number of columns in A and rows in B.
    /// @param n Number of columns in B.
    ///
    /// @pre When m and k are greater than zero, a must reference
    ///     valid storage for at least m × k float elements.
    /// @pre When k and n are greater than zero, b must reference
    ///     valid storage for at least k × n float elements.
    /// @pre When m and n are greater than zero, output must reference
    ///     valid storage for at least m × n float elements.
    /// @pre The output buffer must not overlap either input buffer.
    /// @pre Matrix dimensions and their products must be representable
    ///     by std::size_t.
    ///
    /// @post Each output element contains the dot product of the
    ///       corresponding row of A and column of B.
    ///
    /// @note This is Outrunner's sequential CPU reference implementation.
    /// @note The caller owns all buffers and is responsible for
    ///     providing sufficient storage.
    /// @note If m or n is zero, no output elements are written.
    /// @note If k is zero, every element of a non-empty output matrix
    ///     is assigned 0.0f.
    void matmul(
        const float* a,
        const float* b,
        float* output,
        std::size_t m,
        std::size_t k,
        std::size_t n
    );

} // namespace outrunner::cpu
