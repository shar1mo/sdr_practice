#include <vector>
#include <complex.h>

std::vector<int> ted(std::vector<std::complex<double>> &a, int sps);
std::vector<std::complex<double>> symbol_sync(std::vector<std::complex<double>> &matched, int nsps);
std::vector<std::complex<double>> clock_recovery_mueller_muller(const std::vector<std::complex<double>> &samples, double sps);