#include "cinder/CinderMath.h"
#include <iostream>
#include <algorithm>

using namespace cinder;

int main() {
    double result[4];
    double coeffs[5];
    // coefficients for polynomial with roots 2,2.001,2.002,2.003
    double x1 = 2.0;
    double x2 = 2.001;
    double x3 = 2.002;
    double x4 = 2.003;
    double a = -(x1 + x2 + x3 + x4);
    double b = x1 * (x2 + x3 + x4) + x2 * (x3 + x4) + x3 * x4;
    double c = -x1 * x2 * (x3 + x4) - x3 * x4 * (x1 + x2);
    double d = x1 * x2 * x3 * x4;
    coeffs[4] = 1.0; coeffs[3] = a; coeffs[2] = b; coeffs[1] = c; coeffs[0] = d;
    int n = solveQuartic(coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], result);
    std::cout << "n=" << n << "\n";
    std::sort(result, result + n);
    for(int i=0;i<n;i++) {
        std::cout.precision(16);
        std::cout << result[i] << "\n";
    }
    return 0;
}
