#include <stdint.h>
#include <complex.h>
#include <vector>

// For PlutoSDR
void bpsk(std::vector<int> &in, std::vector<std::complex<double>> &out);
void qpsk(std::vector<int> &in, std::vector<std::complex<double>> &out);
void pam_4(std::vector<int> &in, std::vector<int> &out);
void pam_4_to_qam_4_2(std::vector<int> &in, std::vector<std::complex<double>> &out);

std::vector<std::complex<double>> modulate(std::vector<int> &in, int modulation_order);

void demod_bpsk(std::vector<std::complex<double>> &in, std::vector<int> &out);
std::vector<int> demodulate( std::vector<std::complex<double>> &in, int modulation_order);

