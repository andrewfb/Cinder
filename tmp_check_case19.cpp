#include "cinder/CinderMath.h"
#include <iostream>
#include <algorithm>

int main() {
    double result[4];
    double x1=1.0, x2=1e30, x3=1e30, x4=1e44;
    double a = -(x1+x2+x3+x4);
    double b = x1*(x2+x3+x4) + x2*(x3+x4) + x3*x4;
    double c = -x1*x2*(x3+x4) - x3*x4*(x1+x2);
    double d = x1*x2*x3*x4;
    int n = cinder::solveQuartic(d, c, b, a, 1.0, result);
    std::sort(result, result+n);
    std::cout << "n="<<n<<"\n";
    std::cout.precision(16);
    for(int i=0;i<n;i++) std::cout<<result[i]<<"\n";
    return 0;
}
