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

Path2d createComplexPath() {
    Path2d path;
    path.moveTo(50, 50);
    path.curveTo(100, 200, 200, 200, 250, 50);
    path.quadTo(300, 100, 350, 50);
    path.lineTo(400, 100);
    path.curveTo(450, 150, 450, 200, 400, 250);
    path.lineTo(350, 300);
    path.quadTo(300, 350, 250, 300);
    path.lineTo(200, 250);
    path.close();
    return path;
}

void benchmarkPath2d() {
    const int ITERATIONS = 10000;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> coordDist(0.0f, 500.0f);

    // Create test paths
    std::vector<Path2d> testPaths;
    for(int i = 0; i < 100; ++i) {
        testPaths.push_back(createComplexPath());
    }

    // Generate test points
    std::vector<vec2> testPoints;
    for(int i = 0; i < ITERATIONS; ++i) {
        testPoints.push_back(vec2(coordDist(rng), coordDist(rng)));
    }

    // Benchmark calcPreciseBoundingBox (uses findCubicBezierExtrema with stable solver)
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS / 10; ++i) {
            volatile Rectf bbox = testPaths[i % testPaths.size()].calcPreciseBoundingBox();
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  calcPreciseBoundingBox (×1000)"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark calcDistance
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            const Path2d& path = testPaths[i % testPaths.size()];
            volatile float dist = path.calcDistance(testPoints[i]);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  calcDistance"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark calcClosestPoint
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS; ++i) {
            const Path2d& path = testPaths[i % testPaths.size()];
            volatile vec2 closest = path.calcClosestPoint(testPoints[i]);
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  calcClosestPoint"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark calcLength
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS / 10; ++i) {
            const Path2d& path = testPaths[i % testPaths.size()];
            volatile float length = path.calcLength();
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  calcLength (×1000)"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark getPosition
    {
        BenchmarkTimer timer;
        std::uniform_real_distribution<float> tDist(0.0f, 1.0f);

        for(int i = 0; i < ITERATIONS; ++i) {
            const Path2d& path = testPaths[i % testPaths.size()];
            volatile vec2 pos = path.getPosition(tDist(rng));
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  getPosition"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark getTangent
    {
        BenchmarkTimer timer;
        std::uniform_real_distribution<float> tDist(0.0f, 1.0f);

        for(int i = 0; i < ITERATIONS; ++i) {
            const Path2d& path = testPaths[i % testPaths.size()];
            volatile vec2 tangent = path.getTangent(tDist(rng));
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  getTangent"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }

    // Benchmark subdivide
    {
        BenchmarkTimer timer;

        for(int i = 0; i < ITERATIONS / 100; ++i) {
            const Path2d& path = testPaths[i % testPaths.size()];
            volatile std::vector<vec2> subdivided = path.subdivide();
        }

        double elapsed = timer.elapsedMicroseconds();
        std::cout << std::setw(50) << std::left << "  subdivide (×100)"
                  << std::setw(12) << std::right << elapsed << " μs" << std::endl;
    }
}
