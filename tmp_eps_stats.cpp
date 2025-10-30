#include "cinder/CinderMath.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>

using namespace cinder;

template <typename T>
T eps_rel(T raw, T ref)
{
    return (ref == T(0)) ? std::abs(raw) : std::abs((raw - ref) / ref);
}

template <typename T>
T calc_eps_q(T a, T b, T c, T d, T a1, T b1, T a2, T b2)
{
    T eps_a = eps_rel(a1 + a2, a);
    T eps_b = eps_rel(b1 + a1 * a2 + b2, b);
    T eps_c = eps_rel(b1 * a2 + a1 * b2, c);
    return eps_a + eps_b + eps_c;
}

template <typename T>
T calc_eps_t(T a, T b, T c, T d, T a1, T b1, T a2, T b2)
{
    return calc_eps_q(a, b, c, d, a1, b1, a2, b2) + eps_rel(b1 * b2, d);
}

struct Case { std::string name; double a,b,c,d; };

Case make_case_from_roots(const std::string &name, double x1, double x2, double x3, double x4)
{
    double a = -(x1 + x2 + x3 + x4);
    double b = x1 * (x2 + x3 + x4) + x2 * (x3 + x4) + x3 * x4;
    double c = -x1 * x2 * (x3 + x4) - x3 * x4 * (x1 + x2);
    double d = x1 * x2 * x3 * x4;
    return {name, a, b, c, d};
}

Case make_case_with_coeffs(const std::string &name, double a, double b, double c, double d)
{
    return {name, a, b, c, d};
}

void analyze_case(const Case &cse)
{
    double a1,b1,a2,b2;
    bool ok = factorQuarticInner<double>(cse.a, cse.b, cse.c, cse.d, false, a1, b1, a2, b2);
    double eps = calc_eps_t(cse.a, cse.b, cse.c, cse.d, a1, b1, a2, b2);
    std::cout << cse.name << " unscaled ok=" << ok << " eps_t=" << eps;
    if(ok) {
        double roots[4];
        double r1[2], r2[2];
        int n1 = solveQuadraticStable(1.0, a1, b1, r1);
        int n2 = solveQuadraticStable(1.0, a2, b2, r2);
        int idx=0;
        for(int i=0;i<n1;i++) roots[idx++] = r1[i];
        for(int i=0;i<n2;i++) roots[idx++] = r2[i];
        std::sort(roots, roots + idx);
        std::cout << " roots:";
        std::cout << std::setprecision(16);
        for(int i=0;i<idx;i++) std::cout << ' ' << roots[i];
        std::cout << std::setprecision(6);
    }
    std::cout << '\n';

    const double K_Q = 7.16e76;
    double as = cse.a / K_Q;
    double bs = cse.b / (K_Q * K_Q);
    double cs = cse.c / (K_Q * K_Q * K_Q);
    double ds = cse.d / (K_Q * K_Q * K_Q * K_Q);
    bool ok2 = factorQuarticInner<double>(as, bs, cs, ds, false, a1, b1, a2, b2);
    double eps2 = calc_eps_t(as, bs, cs, ds, a1, b1, a2, b2);
    std::cout << "  scaled ok=" << ok2 << " eps_t=" << eps2;
    if(ok2) {
        double roots[4];
        double r1[2], r2[2];
        int n1 = solveQuadraticStable(1.0, a1, b1, r1);
        int n2 = solveQuadraticStable(1.0, a2, b2, r2);
        int idx=0;
        for(int i=0;i<n1;i++) roots[idx++] = r1[i] * K_Q;
        for(int i=0;i<n2;i++) roots[idx++] = r2[i] * K_Q;
        std::sort(roots, roots + idx);
        std::cout << " roots:";
        std::cout << std::setprecision(16);
        for(int i=0;i<idx;i++) std::cout << ' ' << roots[i];
        std::cout << std::setprecision(6);
    }
    std::cout << '\n';

    bool ok3 = factorQuarticInner<double>(as, bs, cs, ds, true, a1, b1, a2, b2);
    double eps3 = calc_eps_t(as, bs, cs, ds, a1, b1, a2, b2);
    std::cout << "  scaled+rescale ok=" << ok3 << " eps_t=" << eps3;
    if(ok3) {
        double roots[4];
        double r1[2], r2[2];
        int n1 = solveQuadraticStable(1.0, a1, b1, r1);
        int n2 = solveQuadraticStable(1.0, a2, b2, r2);
        int idx=0;
        for(int i=0;i<n1;i++) roots[idx++] = r1[i] * K_Q;
        for(int i=0;i<n2;i++) roots[idx++] = r2[i] * K_Q;
        std::sort(roots, roots + idx);
        std::cout << " roots:";
        std::cout << std::setprecision(16);
        for(int i=0;i<idx;i++) std::cout << ' ' << roots[i];
        std::cout << std::setprecision(6);
    }
    std::cout << '\n';
}

int main()
{
    std::vector<Case> cases;
    cases.push_back(make_case_from_roots("case1", 1., 1e3, 1e6, 1e9));
    cases.push_back(make_case_from_roots("case2", 2., 2.001, 2.002, 2.003));
    cases.push_back(make_case_from_roots("case3", 1e47, 1e49, 1e50, 1e53));
    cases.push_back(make_case_from_roots("case4", -1., 1., 2., 1e14));
    cases.push_back(make_case_from_roots("case5", -2e7, -1., 1., 1e7));
    cases.push_back(make_case_with_coeffs("case23", 1., 1., 3.0/8.0, 1e-3));

    for(const auto &cse : cases)
        analyze_case(cse);

    return 0;
}
