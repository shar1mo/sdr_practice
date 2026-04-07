#include <vector>
#include <complex>
#include <fftw3.h>

#include "ofdm.h"

ofdm_params_t init_ofdm_params()
{
    ofdm_params_t p;
    p.fft_size = 128;
    p.cp_len = p.fft_size / 4;

    for (int i = 0; i < p.fft_size; i++)
    {
        if(i < 6 || i >= p.fft_size - 6){
            continue;
        }
        if(i % 16 == 0){
            p.pilot_carriers.push_back(i);
        } else {
            p.data_carriers.push_back(i);
        }
    }

    return p;
}

std::vector<std::complex<double>> fft(std::vector<std::complex<double>> &in)
{
    int N = in.size();
    std::vector<std::complex<double>> out(N);

    fftw_complex *fftw_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *fftw_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);

    for (int i = 0; i < N; i++)
    {
        fftw_in[i][0] = in[i].real();
        fftw_in[i][1] = in[i].imag();
    }

    fftw_plan p = fftw_plan_dft_1d(N, fftw_in, fftw_out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);

    for (int i = 0; i < N; i++)
    {
        out[i] = std::complex<double>(fftw_out[i][0], fftw_out[i][1]);
    }

    fftw_destroy_plan(p);
    fftw_free(fftw_in);
    fftw_free(fftw_out);

    return out;
}

std::vector<std::complex<double>> ifft(std::vector<std::complex<double>> &in)
{
    int N = in.size();
    std::vector<std::complex<double>> out(N);

    fftw_complex *fftw_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *fftw_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);

    for (int i = 0; i < N; i++)
    {
        fftw_in[i][0] = in[i].real();
        fftw_in[i][1] = in[i].imag();
    }

    fftw_plan p = fftw_plan_dft_1d(N, fftw_in, fftw_out, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(p);

    for (int i = 0; i < N; i++)
    {
        out[i] = std::complex<double>(fftw_out[i][0] / N, fftw_out[i][1] / N);
    }

    fftw_destroy_plan(p);
    fftw_free(fftw_in);
    fftw_free(fftw_out);

    return out;
}

std::vector<std::complex<double>> ofdm_modulate(
    std::vector<std::complex<double>> &data,
    ofdm_params_t &params
)
{
    std::vector<std::complex<double>> tx;
    int idx = 0;

    while(idx < data.size()){
        std::vector<std::complex<double>> freq(params.fft_size, {0.0, 0.0});

        for (int i = 0; i < params.data_carriers.size(); i++)
        {
            if(idx < data.size()){
                freq[params.data_carriers[i]] = data[idx++];
            }
        }

        for (int i = 0; i < params.pilot_carriers.size(); i++)
        {
            freq[params.pilot_carriers[i]] = std::complex<double>(1.0, 0.0);
        }

        std::vector<std::complex<double>> time = ifft(freq);

        for (int i = params.fft_size - params.cp_len; i < params.fft_size; i++)
        {
            tx.push_back(time[i]);
        }

        for (int i = 0; i < params.fft_size; i++)
        {
            tx.push_back(time[i]);
        }
    }

    return tx;
}

std::vector<std::complex<double>> ofdm_demodulate(
    std::vector<std::complex<double>> &rx_samples,
    ofdm_params_t &params
)
{
    std::vector<std::complex<double>> out;

    int sym_len = params.fft_size + params.cp_len;
    int idx = 0;

    while(idx + sym_len <= rx_samples.size()){
        std::vector<std::complex<double>> symbol(params.fft_size);

        for (int i = 0; i < params.fft_size; i++)
        {
            symbol[i] = rx_samples[idx + params.cp_len + i];
        }

        idx += sym_len;

        std::vector<std::complex<double>> freq = fft(symbol);

        for (int i = 0; i < params.data_carriers.size(); i++)
        {
            out.push_back(freq[params.data_carriers[i]]);
        }
    }

    return out;
}