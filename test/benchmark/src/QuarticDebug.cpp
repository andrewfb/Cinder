#include "cinder/CinderMath.h"
#include <iostream>
#include <iomanip>

using namespace ci;

// Copy of factorQuarticInner with debug output
template<typename T>
bool factorQuarticInnerDebug( T a, T b, T c, T d, bool rescale, T &alpha_1, T &beta_1, T &alpha_2, T &beta_2 )
{
    auto eps_rel = [](T raw, T ref) {
        return (ref == T(0)) ? math<T>::abs(raw) : math<T>::abs((raw - ref) / ref);
    };

    auto calc_eps_q = [&](T a1, T b1, T a2, T b2) {
        T eps_a = eps_rel(a1 + a2, a);
        T eps_b = eps_rel(b1 + a1 * a2 + b2, b);
        T eps_c = eps_rel(b1 * a2 + a1 * b2, c);
        return eps_a + eps_b + eps_c;
    };

    auto calc_eps_t = [&](T a1, T b1, T a2, T b2) {
        return calc_eps_q(a1, b1, a2, b2) + eps_rel(b1 * b2, d);
    };

    T disc = T(9) * a * a - T(24) * b;
    T s = (disc >= T(0)) ? (T(-2) * b / (T(3) * a + std::copysign(math<T>::sqrt(disc), a))) : (T(-0.25) * a);

    T a_prime = a + T(4) * s;
    T b_prime = b + T(3) * s * (a + T(2) * s);
    T c_prime = c + s * (T(2) * b + s * (T(3) * a + T(4) * s));
    T d_prime = d + s * (c + s * (b + s * (a + s)));

    T g_prime, h_prime;
    const T K_C = T(3.49e102);
    if( rescale ) {
        T a_prime_s = a_prime / K_C;
        T b_prime_s = b_prime / K_C;
        T c_prime_s = c_prime / K_C;
        T d_prime_s = d_prime / K_C;
        g_prime = a_prime_s * c_prime_s - (T(4) / K_C) * d_prime_s - (T(1) / T(3)) * b_prime_s * b_prime_s;
        h_prime = (a_prime_s * c_prime_s + (T(8) / K_C) * d_prime_s - (T(2) / T(9)) * b_prime_s * b_prime_s)
                * (T(1) / T(3)) * b_prime_s
                - c_prime_s * (c_prime_s / K_C)
                - a_prime_s * a_prime_s * d_prime_s;
    } else {
        g_prime = a_prime * c_prime - T(4) * d_prime - (T(1) / T(3)) * b_prime * b_prime;
        h_prime = (a_prime * c_prime + T(8) * d_prime - (T(2) / T(9)) * b_prime * b_prime) * (T(1) / T(3)) * b_prime
                - c_prime * c_prime
                - a_prime * a_prime * d_prime;
    }

    std::cout << "  rescale=" << rescale << ", g_prime=" << g_prime << ", h_prime=" << h_prime << std::endl;

    if( !std::isfinite(g_prime) || !std::isfinite(h_prime) ) {
        std::cout << "  -> FAIL: g_prime or h_prime not finite" << std::endl;
        return false;
    }

    // We'd need to copy the whole depressed cubic code here...
    // For now just return false to see if we even get here
    std::cout << "  -> continuing..." << std::endl;
    return false;
}

int main() {
    std::cout << "\nQuartic Factorization Debug:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

    // case23: x^4 + x^3 + x^2 + 0.375x + 0.001
    {
        std::cout << "case23: challenging polynomial\n";
        float c4 = 1.0f;
        float c3 = 1.0f;
        float c2 = 1.0f;
        float c1 = 0.375f;
        float c0 = 0.001f;

        float a = c3 / c4;
        float b = c2 / c4;
        float c = c1 / c4;
        float d = c0 / c4;

        std::cout << "  Normalized: a=" << a << ", b=" << b << ", c=" << c << ", d=" << d << std::endl;

        float alpha_1, beta_1, alpha_2, beta_2;
        std::cout << "Attempt 1: no rescaling\n";
        factorQuarticInnerDebug(a, b, c, d, false, alpha_1, beta_1, alpha_2, beta_2);

        const float K_Q = 7.16e76f;
        float a_scaled = a / K_Q;
        float b_scaled = b / (K_Q * K_Q);
        float c_scaled = c / (K_Q * K_Q * K_Q);
        float d_scaled = d / (K_Q * K_Q * K_Q * K_Q);

        std::cout << "\nAttempt 2: with K_Q rescaling, rescale_inner=false\n";
        std::cout << "  Scaled: a=" << a_scaled << ", b=" << b_scaled << ", c=" << c_scaled << ", d=" << d_scaled << std::endl;
        factorQuarticInnerDebug(a_scaled, b_scaled, c_scaled, d_scaled, false, alpha_1, beta_1, alpha_2, beta_2);

        std::cout << "\nAttempt 3: with K_Q rescaling, rescale_inner=true\n";
        factorQuarticInnerDebug(a_scaled, b_scaled, c_scaled, d_scaled, true, alpha_1, beta_1, alpha_2, beta_2);
    }

    std::cout << "\n" << std::endl;

    // case19: (x-1)^2(x-1e44)^2 = x^4 - 2e44*x^3 + 1e88 + 2*x^2 -2x + 1
    {
        std::cout << "case19: (x-1)^2(x-1e44)^2 (expected to overflow float)\n";
        float c4 = 1.0f;
        float c3 = -2e44f;
        float c2 = 1.0f + 1e88f; // Actually this overflows to 1e88
        float c1 = -2.0f;
        float c0 = 1.0f;

        float a = c3 / c4;
        float b = c2 / c4;
        float c = c1 / c4;
        float d = c0 / c4;

        std::cout << "  Normalized: a=" << a << ", b=" << b << ", c=" << c << ", d=" << d << std::endl;

        float alpha_1, beta_1, alpha_2, beta_2;
        std::cout << "Attempt 1: no rescaling\n";
        factorQuarticInnerDebug(a, b, c, d, false, alpha_1, beta_1, alpha_2, beta_2);

        const float K_Q = 7.16e76f;
        float a_scaled = a / K_Q;
        float b_scaled = b / (K_Q * K_Q);
        float c_scaled = c / (K_Q * K_Q * K_Q);
        float d_scaled = d / (K_Q * K_Q * K_Q * K_Q);

        std::cout << "\nAttempt 2: with K_Q rescaling, rescale_inner=false\n";
        std::cout << "  Scaled: a=" << a_scaled << ", b=" << b_scaled << ", c=" << c_scaled << ", d=" << d_scaled << std::endl;
        factorQuarticInnerDebug(a_scaled, b_scaled, c_scaled, d_scaled, false, alpha_1, beta_1, alpha_2, beta_2);

        std::cout << "\nAttempt 3: with K_Q rescaling, rescale_inner=true\n";
        factorQuarticInnerDebug(a_scaled, b_scaled, c_scaled, d_scaled, true, alpha_1, beta_1, alpha_2, beta_2);
    }

    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    return 0;
}
