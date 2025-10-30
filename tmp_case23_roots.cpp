#include "cinder/CinderMath.h"
#include <algorithm>
#include <iostream>

using namespace cinder;

int main() {
    double coeffs[5];
    coeffs[4] = 1.0;
    coeffs[3] = 1.0;
    coeffs[2] = 1.0;
    coeffs[1] = 3.0 / 8.0;
    coeffs[0] = 1e-3;
    double roots[4];
    int n = solveQuartic(coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], roots);
    std::cout << "n=" << n << "\n";
    std::sort(roots, roots + n);
    for(int i=0;i<n;i++) {
        std::cout << roots[i] << "\n";
    }
    return 0;
}
