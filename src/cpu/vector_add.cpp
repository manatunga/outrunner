/// @file vector_add.cpp
/// @brief Sequential CPU implementation of vector addition.
///
/// Provides a deterministic reference computation against which
/// future parallel CPU and GPU implementations can be tested.
#include "outrunner/cpu/vector_add.hpp"

namespace outrunner::cpu {

    /// @brief Implements the sequential CPU vector-addition operation.
    ///
    /// Processes each element independently using a single CPU thread.
    ///
    /// The implementation does not allocate memory, launch threads,
    /// or perform device-specific operations.
    ///
    /// @see outrunner::cpu::add_vector
    void add_vector(
        const float* a,
        const float* b,
        float* output,
        std::size_t count
    ) {
        for (std::size_t i = 0; i < count; i++) {
            output[i] = a[i] + b[i];
        }
    }

} // namespace outrunner::cpu
