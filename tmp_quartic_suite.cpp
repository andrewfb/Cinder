#include "cinder/CinderMath.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace cinder;

bool approx_ok(double actual, double expected, double rel_err, double &out_err) {
    double diff = std::abs(actual - expected);
    double tol = rel_err * std::abs(expected);
    if(expected == 0.0) {
        tol = rel_err;
    }
    out_err = diff;
    return diff <= tol;
}

bool test_with_roots(const std::string &name, const std::array<double, 4> &coeffs, const std::vector<double> &expected, double rel_err) {
    double roots[4];
    int n = solveQuartic<double>(coeffs[3], coeffs[2], coeffs[1], coeffs[0], 1.0, roots);
    std::vector<double> actual(roots, roots + n);
    std::sort(actual.begin(), actual.end());
    std::vector<double> sorted_expected = expected;
    std::sort(sorted_expected.begin(), sorted_expected.end());

    bool ok = true;
    if(n != static_cast<int>(sorted_expected.size())) {
        std::cout << name << ": root count mismatch. got " << n << " expected " << sorted_expected.size() << '\n';
        ok = false;
    }
    int count = std::min<int>(n, sorted_expected.size());
    for(int i = 0; i < count; ++i) {
        double err;
        if(!approx_ok(actual[i], sorted_expected[i], rel_err, err)) {
            std::cout << name << ": root " << i << " mismatch. actual=" << std::setprecision(16) << actual[i]
                      << " expected=" << sorted_expected[i] << " rel_err=" << rel_err << " abs_err=" << err << '\n';
            ok = false;
        }
    }
    if(!ok) {
        std::cout << name << ": actual roots:";
        for(double v : actual) std::cout << ' ' << std::setprecision(16) << v;
        std::cout << " | expected:";
        for(double v : sorted_expected) std::cout << ' ' << std::setprecision(16) << v;
        std::cout << '\n';
    }
    return ok;
}

bool test_vieta(const std::string &name, double x1, double x2, double x3, double x4, const std::vector<double> &expected, double rel_err) {
    double a = -(x1 + x2 + x3 + x4);
    double b = x1 * (x2 + x3) + x2 * (x3 + x4) + x4 * (x1 + x3);
    double c = -x1 * x2 * (x3 + x4) - x3 * x4 * (x1 + x2);
    double d = x1 * x2 * x3 * x4;
    return test_with_roots(name, {a, b, c, d}, expected, rel_err);
}

bool test_vieta_full(const std::string &name, double x1, double x2, double x3, double x4, double rel_err) {
    return test_vieta(name, x1, x2, x3, x4, {x1, x2, x3, x4}, rel_err);
}

int main() {
    bool all_ok = true;
    all_ok &= test_vieta_full("case1", 1., 1e3, 1e6, 1e9, 1e-16);
    all_ok &= test_vieta_full("case2", 2., 2.001, 2.002, 2.003, 1e-6);
    all_ok &= test_vieta_full("case3", 1e47, 1e49, 1e50, 1e53, 2e-16);
    all_ok &= test_vieta_full("case4", -1., 1., 2., 1e14, 1e-16);
    all_ok &= test_vieta_full("case5", -2e7, -1., 1., 1e7, 1e-16);
    all_ok &= test_with_roots("case6", {-9000002.0, -9999981999998.0, 19999982e6, -2e13}, {-1e6, 1e7}, 1e-16);
    all_ok &= test_with_roots("case7", {2000011.0, 1010022000028.0, 11110056e6, 2828e10}, {-7., -4.}, 1e-16);
    all_ok &= test_with_roots("case8", {-100002011.0, 201101022001.0, -102200111000011.0, 11000011e8}, {11., 1e8}, 1e-16);
    all_ok &= test_vieta("case14", 1000., 1000., 1000., 1000., {1000., 1000.}, 1e-16);
    all_ok &= test_vieta("case15", 1e-15, 1000., 1000., 1000., {1e-15, 1000., 1000.}, 1e-15);
    all_ok &= test_vieta_full("case17", 10000., 10001., 10010., 10100., 1e-6);
    all_ok &= test_vieta("case19", 1., 1e30, 1e30, 1e44, {1., 1e30, 1e44}, 1e-16);
    all_ok &= test_vieta_full("case20", 1., 1e7, 1e7, 1e14, 1e-7);
    all_ok &= test_vieta_full("case22", 1., 10., 1e152, 1e154, 3e-16);
    all_ok &= test_with_roots("case23", {1., 1., 3. / 8., 1e-3}, {-0.497314148060048, -0.00268585193995149}, 2e-15);
    const double S = 1e30;
    all_ok &= test_with_roots("case24", {-(1. + 1. / S), 1. / S - S * S, S * S + S, -S}, {-S, 1e-30, 1., S}, 2e-16);

    if(!all_ok) {
        std::cout << "Some quartic tests FAILED" << std::endl;
        return 1;
    }
    std::cout << "All quartic tests passed" << std::endl;
    return 0;
}
