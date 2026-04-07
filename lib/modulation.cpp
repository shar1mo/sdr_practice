#include <vector>
#include <cstdint>
#include <iostream>
#include <complex.h>

#include "modulation.h"

void bpsk(std::vector<int> &in, std::vector<std::complex<double>> &out)
{
    std::complex<double> val;
    for (int i = 0; i < in.size(); i++)
    {
        if(in[i] == 1){
            val = 1.0 + 0j;
        } else {
            val = -1.0 + 0j;
        }
        out.push_back(val);
    }
}

void qpsk(std::vector<int> &in, std::vector<std::complex<double>> &out)
{
    std::complex<double> val;
    double real, imag, k;
    out.resize(in.size() / 2);
    k = 1 / sqrt(2);
    int j = 0;
    for (int i = 0; i < in.size(); i += 2)
    {
        real = k * (1 - 2 * in[i]);
        imag = k * (1 - 2 * in[i + 1]);
        out[j] = std::complex<double>(real, imag);
        j++;
    }
}

void pam_4(std::vector<int> &in, std::vector<int> &out)
{
    // 4-PAM
    // 00 = -3
    // 11 = 3
    // 01 = -1
    // 10 = 1
    out.resize(in.size() / 2);
    int j = 0; 
    for (int i = 0; i < in.size(); i += 2)
    {
        if(in[i] == 0 && in[i+1] == 0){
            out[j] = -3;
        } else if(in[i] == 1 && in[i+1] == 1){
            out[j] = 3;
        } else if(in[i] == 0 && in[i+1] == 1){
            out[j] = -1;
        } else if(in[i] == 1 && in[i+1] == 0){
            out[j] = 1;
        }
        j++;
    }
}

void pam_4_to_qam_4_2(std::vector<int> &in, std::vector<std::complex<double>> &out)
{
    std::complex<double> val;
    double real, imag, k;
    out.resize(in.size());
    for (int i = 0; i < in.size(); i++){
        if(in[i] == -3){
            val = {-3, -3};
        } else if(in[i] == 3){
            val = {3, 3};
        } else if(in[i] == -1){
            val = {-1, -1};
        }
        else {
            val = {1, 1};
        }
        out[i] = val;
    }
}


std::vector<std::complex<double>> modulate(std::vector<int> &in, int modulation_order)
{
    std::vector<std::complex<double>> iq_samples;
    switch(modulation_order){
        case 1:
            bpsk(in, iq_samples);
            break;
        case 2:
            if(in.size() % 2 != 0){
                std::cout << "Error, array % 2 != 0" << std::endl;
                return iq_samples;
            }
            else {
                qpsk(in, iq_samples);
            }
            break;
    }

    return iq_samples;
}

std::vector<int> demodulate(std::vector<std::complex<double>> &in, int modulation_order)
{
    std::vector<int> bit_array;
    switch(modulation_order){
        case 1:
            demod_bpsk(in, bit_array);
            break;
    }

    return bit_array;
}

void demod_bpsk(std::vector<std::complex<double>> &in, std::vector<int> &out)
{
    int val;
    for (int i = 0; i < in.size(); i++)
    {
        if(in[i].real() >= 0){
            val = 1;
        } else {
            val = 0;
        }
        out.push_back(val);
    }
}