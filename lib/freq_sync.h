#include <vector>
#include <complex.h>

std::vector<std::complex<double>> coarse_freq_sync(std::vector<std::complex<double>> &in, double coarse_freq, int buffer_size, int sample_rate);
double coarse_max_freq_calculation(std::vector<std::complex<double>> &in, int buffer_size, int sample_rate);
std::vector<std::complex<double>> costas_loop(std::vector<std::complex<double>> &in);
std::vector<std::complex<double>> costas_loop_bpsk(const std::vector<std::complex<double>>& samples);