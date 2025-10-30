#include "cinder/CinderMath.h"
#include <iostream>
#include <algorithm>

using namespace cinder;

int main() {
    double x1=1.0, x2=1e30, x3=1e30, x4=1e44;
    double a = -(x1+x2+x3+x4);
    double b = x1*(x2+x3+x4) + x2*(x3+x4) + x3*x4;
    double c = -x1*x2*(x3+x4) - x3*x4*(x1+x2);
    double d = x1*x2*x3*x4;

    const double K_Q = 7.16e76;
    double a_scaled = a / K_Q;
    double b_scaled = b / (K_Q * K_Q);
    double c_scaled = c / (K_Q * K_Q * K_Q);
    double d_scaled = d / (K_Q * K_Q * K_Q * K_Q);

    double alpha1, beta1, alpha2, beta2;
    bool ok = factorQuarticInner<double>(a_scaled, b_scaled, c_scaled, d_scaled, false, alpha1, beta1, alpha2, beta2);
    std::cout << "factor ok="<<ok<<"\n";
    std::cout << "alpha1="<<alpha1<<" beta1="<<beta1<<" alpha2="<<alpha2<<" beta2="<<beta2<<"\n";
    double roots1[2], roots2[2];
    int n1 = solveQuadraticStable(1.0, alpha1, beta1, roots1);
    int n2 = solveQuadraticStable(1.0, alpha2, beta2, roots2);
    std::cout << "n1="<<n1<<" roots1 before scale:";
    for(int i=0;i<n1;i++) std::cout<<" "<<roots1[i];
    std::cout<<"\n";
    std::cout << "n2="<<n2<<" roots2 before scale:";
    for(int i=0;i<n2;i++) std::cout<<" "<<roots2[i];
    std::cout<<"\n";
    std::cout << "after scale:";
    for(int i=0;i<n1;i++) std::cout<<" "<<roots1[i]*K_Q;
    for(int i=0;i<n2;i++) std::cout<<" "<<roots2[i]*K_Q;
    std::cout<<"\n";
    return 0;
}
