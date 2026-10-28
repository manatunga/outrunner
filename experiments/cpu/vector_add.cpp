// OR-E1: CPU Vector Addition

#include <iostream>

int main() {
    int a[4] = {1, 2, 3, 4};
    int b[4] = {5, 6, 7, 8};
    int c[4];

    std::cout << "Vector addition result:" << std::endl;

    std::cout << "[";
    for (int i = 0; i < 4; i++) {
        c[i] = a[i] + b[i];
        std::cout << c[i];

        if (i < 3) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    std::cout << "Size of int: " << sizeof(int) << std::endl;
    std::cout << "Size of a: " << sizeof(a) << std::endl;

    std::cout << "Memory address of a[0]: " << &a[0] << '\n';
    std::cout << "Memory address of a[1]: " << &a[1] << '\n';
    std::cout << "Memory address of a[2]: " << &a[2] << '\n';
    std::cout << "Memory address of a[3]: " << &a[3] << '\n';

    return 0;
}