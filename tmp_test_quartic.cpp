#include "cinder/CinderMath.h"
#include <iostream>
#include <iomanip>

using namespace cinder;

int main() {
    const double K_Q = 7.16e76;
    double a = -1.00000000000002e+44 / K_Q;
    double b = 2.0000000000000102e+74 / (K_Q * K_Q);
    double c = -1.0000000000000002e+104 / (K_Q * K_Q * K_Q);
    double d = 1.0000000000000002e+104 / (K_Q * K_Q * K_Q * K_Q);
    double alpha_1, beta_1, alpha_2, beta_2;
    factorQuarticInner<double>(a, b, c, d, true, alpha_1, beta_1, alpha_2, beta_2);
    double roots1[2], roots2[2];
    int n1 = solveQuadraticStable(1.0, alpha_1, beta_1, roots1);
    int n2 = solveQuadraticStable(1.0, alpha_2, beta_2, roots2);
    std::cout << std::setprecision(18);
    for(int i=0;i<n1;i++) std::cout << "r1=" << roots1[i] * K_Q << '\n';
    for(int i=0;i<n2;i++) std::cout << "r2=" << roots2[i] * K_Q << '\n';
    return 0;
}
