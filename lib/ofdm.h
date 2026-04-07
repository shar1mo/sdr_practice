#pragma once

#include <vector>
#include <complex>

struct ofdm_params_t {
    int fft_size;
    int cp_len;
    std::vector<int> data_carriers;
    std::vector<int> pilot_carriers;
};

ofdm_params_t init_ofdm_params();

std::vector<std::complex<double>> ofdm_modulate(
    std::vector<std::complex<double>> &data,
    ofdm_params_t &params
);

std::vector<std::complex<double>> ofdm_demodulate(
    std::vector<std::complex<double>> &rx_samples,
    ofdm_params_t &params
);

std::vector<std::complex<double>> fft(std::vector<std::complex<double>> &in);
std::vector<std::complex<double>> ifft(std::vector<std::complex<double>> &in);