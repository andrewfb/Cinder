#include "cinder/CinderMath.h"
#include <iostream>
#include <cmath>

using namespace cinder;

template <typename T>
T eps_rel(T raw, T ref) {
    return (ref == T(0)) ? std::abs(raw) : std::abs((raw - ref) / ref);
}

template <typename T>
T calc_eps_t_outer(T a, T b, T c, T d, T a1, T b1, T a2, T b2) {
    auto calc_eps_q = [&](T aa1, T bb1, T aa2, T bb2) {
        T eps_a = eps_rel(aa1 + aa2, a);
        T eps_b = eps_rel(bb1 + aa1 * aa2 + bb2, b);
        T eps_c = eps_rel(bb1 * aa2 + aa1 * bb2, c);
        return eps_a + eps_b + eps_c;
    };
    return calc_eps_q(a1, b1, a2, b2) + eps_rel(b1 * b2, d);
}

int main() {
    double a = 1.0;
    double b = 1.0;
    double c = 3.0 / 8.0;
    double d = 1e-3;
    double alpha1, beta1, alpha2, beta2;
    bool ok = factorQuarticInner<double>(a, b, c, d, false, alpha1, beta1, alpha2, beta2);
    std::cout << "pass1 ok=" << ok << " eps_t=" << calc_eps_t_outer(a,b,c,d,alpha1,beta1,alpha2,beta2) << "\n";
    std::cout << "alpha1="<<alpha1<<" beta1="<<beta1<<" alpha2="<<alpha2<<" beta2="<<beta2<<"\n";

    const double K_Q = 7.16e76;
    double a_s = a / K_Q;
    double b_s = b / (K_Q * K_Q);
    double c_s = c / (K_Q * K_Q * K_Q);
    double d_s = d / (K_Q * K_Q * K_Q * K_Q);
    double alpha1s, beta1s, alpha2s, beta2s;
    bool ok2 = factorQuarticInner<double>(a_s, b_s, c_s, d_s, false, alpha1s, beta1s, alpha2s, beta2s);
    std::cout << "pass2 ok=" << ok2 << " eps_t=" << calc_eps_t_outer(a_s,b_s,c_s,d_s,alpha1s,beta1s,alpha2s,beta2s) << "\n";
    std::cout << "alpha1s="<<alpha1s<<" beta1s="<<beta1s<<" alpha2s="<<alpha2s<<" beta2s="<<beta2s<<"\n";

    bool ok3 = factorQuarticInner<double>(a_s, b_s, c_s, d_s, true, alpha1s, beta1s, alpha2s, beta2s);
    std::cout << "pass3 ok(rescale)=" << ok3 << " eps_t=" << calc_eps_t_outer(a_s,b_s,c_s,d_s,alpha1s,beta1s,alpha2s,beta2s) << "\n";
    std::cout << "alpha1s="<<alpha1s<<" beta1s="<<beta1s<<" alpha2s="<<alpha2s<<" beta2s="<<beta2s<<"\n";

    return 0;
}
