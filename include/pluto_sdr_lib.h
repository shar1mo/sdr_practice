#ifndef PLUTO_SDR_LIB_H
#define PLUTO_SDR_LIB_H

#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h> //printf
#include <stdlib.h> //free
#include <stdint.h>
#include <complex.h>
#include <vector>
#include <complex>

#define BUFFER_SIZE 1920 * 2

typedef struct sdr_config_s{
    const char *name; // usb or ip adress (изменено на const char*)
    bool is_tx;
    int buffer_size;
    double tx_bandwidth;
    double tx_sample_rate;
    double tx_carrier_freq;
    double tx_gain;

    double rx_bandwidth;
    double rx_sample_rate;
    double rx_carrier_freq;
    double rx_gain;
    size_t channels[1] = {0} ;
} sdr_config_t;

typedef struct sdr_phy_s{
    int Nsps = 16;
    long long rx_timeNs;
    int16_t pluto_rx_buffer[2 * BUFFER_SIZE];
    int16_t pluto_tx_buffer[2 * BUFFER_SIZE];
    std::vector< std::complex<double> > raw_samples;
    std::vector< std::complex<double> > matched_samples;
    std::vector< std::complex<double> > symb_sync_samples;
    std::vector< std::complex<double> > costas_sync_samples;
} sdr_phy_t;

typedef struct test_set_s{
    int Nbit = 10;
    int N;
    int MO = 1;
    int symb_size = 16;
    std::vector<int> xAxis;
    std::vector<int> xAxis_upsampled;
    std::vector<int> barker;
    std::vector<int> bit_array = {0, 1, 1, 0, 1, 1, 0, 0, 0, 1};

    std::vector<std::complex<double>> modulated_array;
    std::vector<std::complex<double>> upsampled_bit_array;
    std::vector<std::complex<double>> pulse_shaped;
    std::vector<std::complex<int>> samples_to_tx;

    std::vector<std::complex<double>> channel_samples;
    std::vector<std::complex<double>> matched_samples;
    std::vector<std::complex<double>> coarsed_samples;
    std::vector<std::complex<double>> shifted_before_cl;
    std::vector<std::complex<double>> matched_squared_samples;
    std::vector<std::complex<double>> ted_samples;
    std::vector<std::complex<double>> quantalph;
    std::vector<std::complex<double>> costas_samples;
    std::vector<std::complex<double>> fft_in_samples;
    std::vector<std::complex<double>> fft_out_samples;
    std::vector<int> demod_bit_array;
    std::vector<double> barker_corr_real;
    std::vector<double> barker_corr_imag;
    std::vector<std::complex<double>> barker_correlation;
    std::vector<double> fft_out_abs;
    std::vector<int> ted_err_idx;
    std::vector<int> ted_indexes; 
} test_set_t;

// Инициализация
typedef struct sdr_global_s{
    bool running;
    test_set_t test_set;
    test_set_t test_rx_sdr;
    test_set_t test_bpsk_barker13;
    sdr_config_t sdr_config;
    sdr_phy_t phy;
    SoapySDRDevice *sdr;
    SoapySDRStream *rxStream;
    SoapySDRStream *txStream;
} sdr_global_t;

// Прототипы функций
struct SoapySDRDevice *setup_pluto_sdr(sdr_config_t *config);
struct SoapySDRStream *setup_stream(struct SoapySDRDevice *sdr, sdr_config_t *config, bool isRx);

void close_pluto_sdr(sdr_global_t *sdr);

// Работа с буфферами
void fill_test_tx_buffer(int16_t *buffer, int size);

// Преобразование сэмплов из двух массивов (I[N], Q[N]) в вид Pluto (buff[N*2] = {I, Q, I, Q, ..., I, Q})
void transform_to_pluto_type_smples(std::vector<double> &I_part, std::vector<double> &Q_part, int16_t *buffer);
void transform_from_pluto_type_samples(std::vector<double> &I_part, std::vector<double> &Q_part, int16_t *buffer);

void prepare_test_tx_buffer(sdr_global_t *sdr);
void calculate_test_set(sdr_global_t *sdr);
void test_rx_sdr(sdr_global_t *sdr);
void test_rx_bpsk_barker13(sdr_global_t *sdr);

void run_sdr(sdr_global_t *sdr);

#endif // PLUTO_SDR_LIB_H