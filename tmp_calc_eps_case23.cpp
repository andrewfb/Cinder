#include <iostream>
#include <cmath>

template <typename T>
T eps_rel(T raw, T ref)
{
    return (ref == T(0)) ? std::abs(raw) : std::abs((raw - ref) / ref);
}

template <typename T>
T calc_eps_t(T a, T b, T c, T d, T a1, T b1, T a2, T b2)
{
    T eps_a = eps_rel(a1 + a2, a);
    T eps_b = eps_rel(b1 + a1 * a2 + b2, b);
    T eps_c = eps_rel(b1 * a2 + a1 * b2, c);
    T eps_d = eps_rel(b1 * b2, d);
    return eps_a + eps_b + eps_c + eps_d;
}

int main() {
    double a=1.0, b=1.0, c=3.0/8.0, d=1e-3;
    double alpha1=0.5;
    double beta1=0.0013357121693324022;
    double alpha2=0.5;
    double beta2=0.7486642878306676;
    std::cout << "eps_t="<<calc_eps_t(a,b,c,d,alpha1,beta1,alpha2,beta2)<<"\n";
    return 0;
}
