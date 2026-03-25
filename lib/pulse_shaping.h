#include <vector>
#include <complex.h>

std::vector<std::complex<double>> pulse_shaping(std::vector<std::complex<double>> &vec, int type, int sps);
std::vector<std::complex<double>> upsample(std::vector<std::complex<double>> &vec, int samples_per_symbol);
std::vector<double> srrc(int syms, double beta, int P, double t_off);
std::vector<double> hamming(double M);