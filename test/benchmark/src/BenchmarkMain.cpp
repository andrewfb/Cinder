#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>

// Forward declarations of benchmark functions
void benchmarkQuadraticSolver();
void benchmarkBezierUtilities();
void benchmarkPath2d();

class BenchmarkTimer {
public:
    BenchmarkTimer(const std::string& name) : mName(name), mStart(std::chrono::steady_clock::now()) {}

    ~BenchmarkTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - mStart).count();
        std::cout << std::setw(50) << std::left << mName
                  << std::setw(12) << std::right << duration << " μs" << std::endl;
    }

private:
    std::string mName;
    std::chrono::steady_clock::time_point mStart;
};

void printHeader() {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  Cinder Path2d Math Benchmarks (Release Build)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << std::setw(50) << std::left << "Benchmark"
              << std::setw(12) << std::right << "Time" << std::endl;
    std::cout << "───────────────────────────────────────────────────────────────────────────\n";
}

void printSection(const std::string& section) {
    std::cout << "\n" << section << ":\n";
    std::cout << "───────────────────────────────────────────────────────────────────────────\n";
}

void printFooter() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "\nNote: Lower times are better. Results may vary by platform and CPU.\n";
    std::cout << "Run multiple times for consistent measurements.\n\n";
}

int main(int argc, char* argv[]) {
    printHeader();

    printSection("Quadratic Solver");
    benchmarkQuadraticSolver();

    printSection("Bezier Utilities");
    benchmarkBezierUtilities();

    printSection("Path2d Operations");
    benchmarkPath2d();

    printFooter();

    return 0;
}
