#include <vector>
#include <iostream>
#include <complex.h>
#include "math_operations.h"

std::vector<std::complex<double>> upsample(std::vector<std::complex<double>> &vec, int samples_per_symbol)
{
    std::vector<std::complex<double>> upsampled;
    upsampled.resize(vec.size() * samples_per_symbol);
    int j = 0;
    for (int i = 0; i < upsampled.size(); i += samples_per_symbol)
    {
        upsampled[i] = vec[j];
        j++;
    }
    return upsampled;
}

std::vector<std::complex<double>> pulse_shaping(std::vector<std::complex<double>> &vec, int type, int sps)
{
    std::vector<std::complex<double>> iq_samples;
    switch(type){
        case 0:
            std::vector<double> filter;
            filter.resize(sps);
            for (int i = 0; i < filter.size(); i++)
            {
                filter[i] = 1.0f;
            }
            iq_samples = convolve(vec, filter);
            break;
    }

    return iq_samples;
}

std::vector<double> srrc(int syms, double beta, int P, double t_off = 0) {
    // s = (4*beta/sqrt(P)) scales SRRC
    // syms  = Half of Total Number of Symbols
    int length_SRRC = P * syms * 2;
    double start = (-length_SRRC / 2.0) + 1e-8 + t_off;
    double stop = (length_SRRC / 2.0) + 1e-8 + t_off;
    double step = 1.0;

    std::vector<double> k;
    for (double val = start; val <= stop + 1; val += step) {
        k.push_back(val);
    }

    if (beta == 0) {
        beta = 1e-8;
    }

    std::vector<double> s;
    s.reserve(k.size());

    const double pi = M_PI;

    for (auto val : k) {
        double denom = pi * (1 - 16 * std::pow(beta * val / P, 2));
        double numerator = std::cos((1 + beta) * pi * val / P) + 
                           (std::sin((1 - beta) * pi * val / P) / (4 * beta * val / P));
        s.push_back((4 * beta / std::sqrt(P)) * numerator / denom);
    }

    return s;
}

std::vector<double> hamming(double M) {
    if (M < 1) {
        return std::vector<double>{};
    }
    if (M == 1) {
        return std::vector<double>{1.0};
    }
    std::vector<double> result;
    for (int n = static_cast<int>(1 - M); n < static_cast<int>(M); n += 2) {
        result.push_back(0.54 + 0.46 * std::cos(M_PI * n / (M - 1)));
    }
    return result;
}