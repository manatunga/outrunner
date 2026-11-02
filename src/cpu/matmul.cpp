/// @file matmul.cpp
/// @brief Sequential CPU implementation of matrix multiplication.
///
/// Implements the row-by-column algorithm using three nested loops.
/// Prioritizes correctness and readability over optimizations such as
/// cache blocking, SIMD vectorization, and multithreaded execution.
#include "outrunner/cpu/matmul.hpp"

namespace outrunner::cpu {

    /// @brief Implements sequential multiplication of two floating-point matrices.
    ///
    /// Iterates over each output row and column, accumulating the
    /// products of corresponding elements from the relevant row of A
    /// and column of B.
    ///
    /// @param a Pointer to the first input matrix (m × k).
    /// @param b Pointer to the second input matrix (k × n).
    /// @param output Pointer to the destination matrix (m × n).
    /// @param m Number of rows in the first matrix.
    /// @param k Number of columns in the first matrix and rows in the second.
    /// @param n Number of columns in the second matrix.
    ///
    /// @note Uses contiguous row-major matrix storage.
    /// @note Executes sequentially on a single CPU thread.
    /// @note Does not allocate memory or perform input validation.
    ///
    /// @see outrunner::cpu::matmul
    void matmul(
        const float* a,
        const float* b,
        float* output,
        std::size_t m,
        std::size_t k,
        std::size_t n
    ) {
        for (std::size_t i = 0; i < m; i++) {
            for (std::size_t j = 0; j < n; j++) {
                float sum{0.0f};

                for (std::size_t p = 0; p < k; p++) {
                    sum += a[i * k + p] * b[p * n + j];
                }

                output[i * n + j] = sum;
            }
        }
    }

} // namespace outrunner::cpu
