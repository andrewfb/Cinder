#include "cinder/CinderMath.h"
#include <iostream>
#include <iomanip>

using namespace ci;

int main() {
    std::cout << std::setprecision(10);

    // case23: x^4 + x^3 + x^2 + 0.375x + 0.001
    float c0 = 0.001f, c1 = 0.375f, c2 = 1.0f, c3 = 1.0f, c4 = 1.0f;

    std::cout << "case23: x^4 + x^3 + x^2 + 0.375x + 0.001\n\n";

    float result[4];
    int n = solveQuartic(c0, c1, c2, c3, c4, result);

    std::cout << "\nResult: " << n << " roots\n";
    for (int i = 0; i < n; ++i) {
        std::cout << "  root[" << i << "] = " << result[i] << std::endl;
    }

    // Now test if the roots are actually correct by evaluating the polynomial
    if (n > 0) {
        std::cout << "\nVerification:\n";
        for (int i = 0; i < n; ++i) {
            float x = result[i];
            float val = c4*x*x*x*x + c3*x*x*x + c2*x*x + c1*x + c0;
            std::cout << "  P(" << x << ") = " << val << std::endl;
        }
    }

    // Expected roots from reference (Rust) implementation
    std::cout << "\nExpected roots (from reference): -0.497314148, -0.00268585194\n";

    return 0;
}
