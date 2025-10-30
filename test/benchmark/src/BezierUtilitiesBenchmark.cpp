#include "cinder/CinderMath.h"
#include "cinder/Path2d.h"
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

void benchmarkBezierUtilities() {
    const int ITERATIONS = 100000;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> coordDist(0.0f, 1000.0f);
    std::uniform_real_distribution<float> tDist(0.0f, 1.0f);

    // Generate test curves
    std::vector<vec2> quadControlPoints;
    std::vector<vec2> cubicControlPoints;
    std::vector<float> tValues;

    for(int i = 0; i < ITERATIONS; ++i) {
        quadControlPoints.push_back(vec2(coordDist(rng), coordDist(rng)));
        cubicControlPoints.push_back(vec2(coordDist(rng), coordDist(rng)));
        tValues.push_back(tDist(rng));
    }

    // Ensure we have enough control points
    while(quadControlPoints.size() < ITERATIONS * 3) {
        quadControlPoints.push_back(vec2(coordDist(rng), coordDist(rng)));
    }
    while(cubicControlPoints.size() < ITERATIONS * 4) {
        cubicControlPoints.push_back(vec2(coordDist(rng), coordDist(rng)));
    }

    // Volatile accumulator to prevent dead code elimination
    volatile float sink = 0.0f;

    // Benchmark quadratic evaluation (new CinderMath function)
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 cp[3] = { quadControlPoints[i*3], quadControlPoints[i*3+1], quadControlPoints[i*3+2] };
            vec2 result = evaluateQuadraticBezier(cp, tValues[i]);
            sink += result.x + result.y;
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  evaluateQuadraticBezier"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark cubic evaluation (new CinderMath function)
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 cp[4] = { cubicControlPoints[i*4], cubicControlPoints[i*4+1],
                          cubicControlPoints[i*4+2], cubicControlPoints[i*4+3] };
            vec2 result = evaluateCubicBezier(cp, tValues[i]);
            sink += result.x + result.y;
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  evaluateCubicBezier"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark quadratic derivative
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 cp[3] = { quadControlPoints[i*3], quadControlPoints[i*3+1], quadControlPoints[i*3+2] };
            vec2 result = derivativeQuadraticBezier(cp, tValues[i]);
            sink += result.x + result.y;
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  derivativeQuadraticBezier"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark cubic derivative
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 cp[4] = { cubicControlPoints[i*4], cubicControlPoints[i*4+1],
                          cubicControlPoints[i*4+2], cubicControlPoints[i*4+3] };
            vec2 result = derivativeCubicBezier(cp, tValues[i]);
            sink += result.x + result.y;
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  derivativeCubicBezier"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark quadratic extrema finding (uses new stable solver)
    {
        BenchmarkTimer timer;
        volatile int totalExtrema = 0;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 cp[3] = { quadControlPoints[i*3], quadControlPoints[i*3+1], quadControlPoints[i*3+2] };
            float resultT[2];
            totalExtrema += findQuadraticBezierExtrema(cp, resultT);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  findQuadraticBezierExtrema"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark cubic extrema finding (uses new stable solver)
    {
        BenchmarkTimer timer;
        volatile int totalExtrema = 0;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 cp[4] = { cubicControlPoints[i*4], cubicControlPoints[i*4+1],
                          cubicControlPoints[i*4+2], cubicControlPoints[i*4+3] };
            float resultT[4];
            totalExtrema += findCubicBezierExtrema(cp, resultT);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  findCubicBezierExtrema"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark quadratic subdivision
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 src[3] = { quadControlPoints[i*3], quadControlPoints[i*3+1], quadControlPoints[i*3+2] };
            vec2 dst[5];
            subdivideQuadraticBezier(src, dst, tValues[i]);
            sink += dst[0].x + dst[2].x + dst[4].x;
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  subdivideQuadraticBezier"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark cubic subdivision
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            vec2 src[4] = { cubicControlPoints[i*4], cubicControlPoints[i*4+1],
                           cubicControlPoints[i*4+2], cubicControlPoints[i*4+3] };
            vec2 dst[7];
            subdivideCubicBezier(src, dst, tValues[i]);
            sink += dst[0].x + dst[3].x + dst[6].x;
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  subdivideCubicBezier"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }
}
