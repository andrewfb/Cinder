#include "cinder/CinderMath.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>

using namespace ci;

class BenchmarkTimer {
public:
    BenchmarkTimer() : mStart(std::chrono::steady_clock::now()) {}

    double elapsedMicroseconds() {
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - mStart).count();
    }

private:
    std::chrono::steady_clock::time_point mStart;
};

void benchmarkQuadraticSolver() {
    const int ITERATIONS = 100000;
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(-10.0f, 10.0f);

    // Generate test data
    std::vector<std::array<float, 3>> testCases;
    testCases.reserve(ITERATIONS);
    for(int i = 0; i < ITERATIONS; ++i) {
        float a = dist(rng);
        float b = dist(rng);
        float c = dist(rng);
        if(std::abs(a) < 0.01f) a = 1.0f; // Avoid degenerate cases
        testCases.push_back({a, b, c});
    }

    // Benchmark original solver
    {
        BenchmarkTimer timer;
        float result[2];
        volatile int totalRoots = 0; // volatile prevents optimization

        for(const auto& tc : testCases) {
            totalRoots += solveQuadratic(tc[0], tc[1], tc[2], result);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  solveQuadratic (original)"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark new stable solver
    {
        BenchmarkTimer timer;
        float result[2];
        volatile int totalRoots = 0;

        for(const auto& tc : testCases) {
            totalRoots += solveQuadraticStable(tc[0], tc[1], tc[2], result);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  solveQuadraticStable (new)"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark with challenging cases (b² >> 4ac)
    std::vector<std::array<float, 3>> challengingCases;
    challengingCases.reserve(ITERATIONS);
    for(int i = 0; i < ITERATIONS; ++i) {
        float a = 1.0f;
        float b = 100.0f + dist(rng) * 10.0f; // Large b
        float c = 0.01f; // Small c, so b² >> 4ac
        challengingCases.push_back({a, b, c});
    }

    {
        BenchmarkTimer timer;
        float result[2];
        volatile int totalRoots = 0;

        for(const auto& tc : challengingCases) {
            totalRoots += solveQuadratic(tc[0], tc[1], tc[2], result);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  solveQuadratic (b²>>4ac)"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    {
        BenchmarkTimer timer;
        float result[2];
        volatile int totalRoots = 0;

        for(const auto& tc : challengingCases) {
            totalRoots += solveQuadraticStable(tc[0], tc[1], tc[2], result);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  solveQuadraticStable (b²>>4ac)"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }
}
