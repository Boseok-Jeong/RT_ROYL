#include "../../lib/v1/rtweekend.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

inline double pdf(double x) {
	return 0.5;
}

int main() {
	int N = 1000000;
	auto sum = 0.0;
	for (int i = 0; i < N; i++) {
		auto x = random_double(0, 2);
		sum += x * x / pdf(x);
	}

	std::cout << std::fixed << std::setprecision(12);
	std::cout << "I = "
		<< sum / N << '\n';
}