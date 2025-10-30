#include "cinder/CinderMath.h"
#include <iostream>
#include <algorithm>

using namespace cinder;

template <typename T>
T eps_rel(T raw, T ref) {
    return (ref == T(0)) ? std::abs(raw) : std::abs((raw - ref) / ref);
}

template <typename T>
T calc_eps_t(T a, T b, T c, T d, T a1, T b1, T a2, T b2) {
    T eps_a = eps_rel(a1 + a2, a);
    T eps_b = eps_rel(b1 + a1 * a2 + b2, b);
    T eps_c = eps_rel(b1 * a2 + a1 * b2, c);
    T eps_d = eps_rel(b1 * b2, d);
    return eps_a + eps_b + eps_c + eps_d;
}

int main() {
    double x1=1.0, x2=1e30, x3=1e30, x4=1e44;
    double a = -(x1+x2+x3+x4);
    double b = x1*(x2+x3+x4) + x2*(x3+x4) + x3*x4;
    double c = -x1*x2*(x3+x4) - x3*x4*(x1+x2);
    double d = x1*x2*x3*x4;

    double alpha1, beta1, alpha2, beta2;
    bool ok_unscaled = factorQuarticInner<double>(a, b, c, d, false, alpha1, beta1, alpha2, beta2);
    double eps_unscaled = calc_eps_t(a,b,c,d,alpha1,beta1,alpha2,beta2);
    std::cout << "unscaled ok="<<ok_unscaled<<" eps_t="<<eps_unscaled<<"\n";
    if(ok_unscaled) {
        std::cout << "alpha1="<<alpha1<<" beta1="<<beta1<<" alpha2="<<alpha2<<" beta2="<<beta2<<"\n";
    }

    const double K_Q = 7.16e76;
    double as = a / K_Q;
    double bs = b / (K_Q*K_Q);
    double cs = c / (K_Q*K_Q*K_Q);
    double ds = d / (K_Q*K_Q*K_Q*K_Q);

    bool ok_scaled = factorQuarticInner<double>(as, bs, cs, ds, false, alpha1, beta1, alpha2, beta2);
    double eps_scaled = calc_eps_t(as,bs,cs,ds,alpha1,beta1,alpha2,beta2);
    std::cout << "scaled ok="<<ok_scaled<<" eps_t="<<eps_scaled<<"\n";
    if(ok_scaled) {
        std::cout << "alpha1="<<alpha1<<" beta1="<<beta1<<" alpha2="<<alpha2<<" beta2="<<beta2<<"\n";
    }

    bool ok_scaled_res = factorQuarticInner<double>(as, bs, cs, ds, true, alpha1, beta1, alpha2, beta2);
    double eps_scaled_res = calc_eps_t(as,bs,cs,ds,alpha1,beta1,alpha2,beta2);
    std::cout << "scaled+rescale ok="<<ok_scaled_res<<" eps_t="<<eps_scaled_res<<"\n";
    if(ok_scaled_res) {
        std::cout << "alpha1="<<alpha1<<" beta1="<<beta1<<" alpha2="<<alpha2<<" beta2="<<beta2<<"\n";
    }

    return 0;
}
