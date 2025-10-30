#include "include/cinder/CinderMath.h"
#include <iostream>
#include <iomanip>

int main() {
    double coeffs[5] = {1e-3, 3.0/8.0, 1.0, 1.0, 1.0};
    double a = coeffs[3] / coeffs[4];
    double b = coeffs[2] / coeffs[4];
    double c = coeffs[1] / coeffs[4];
    double d = coeffs[0] / coeffs[4];
    double alpha1, beta1, alpha2, beta2;
    bool ok = cinder::factorQuarticInner<double>(a, b, c, d, false, alpha1, beta1, alpha2, beta2);
    std::cout << std::boolalpha << "ok=" << ok << "\n";
    if(ok) {
        std::cout << std::setprecision(18);
        std::cout << "alpha1=" << alpha1 << " beta1=" << beta1 << "\n";
        std::cout << "alpha2=" << alpha2 << " beta2=" << beta2 << "\n";
    }
    return 0;
}
