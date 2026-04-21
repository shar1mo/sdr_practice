#ifndef PLUTO_UI_H
#define PLUTO_UI_H

#include <vector>
#include <complex>

// Forward declaration - избегаем двойного включения
struct sdr_global_s;
typedef struct sdr_global_s sdr_global_t;

struct test_set_s;
typedef struct test_set_s test_set_t;

void run_gui(sdr_global_t *sdr);
void show_global_menu_bar(sdr_global_t *sdr);
void show_properties_window(sdr_global_t *sdr);
void show_main_window(sdr_global_t *sdr);
void show_tx_data(sdr_global_t *sdr);
void test_rx_from_sdr(sdr_global_t *sdr, test_set_t &test_set);
void test_rx_from_sdr_matched_filter(sdr_global_t *sdr, test_set_t &test_set);
void test_rx_from_sdr_coarse_freq_sync(sdr_global_t *sdr);
void test_rx_from_sdr_symbol_sync(sdr_global_t *sdr, test_set_t &test_set);
void test_rx_from_sdr_freq_sync(sdr_global_t *sdr, test_set_t &test_set);
void test_rx_from_sdr_barker_corr(sdr_global_t *sdr, test_set_t &test_set);
void test_rx_from_sdr_barker_demodulation(sdr_global_t *sdr, test_set_t &test_set);
void test_pulse_shaping(sdr_global_t *sdr);
void test_sinc(sdr_global_t *sdr);
void test_srrc(sdr_global_t *sdr);
void test_hamming(sdr_global_t *sdr);
void show_iq_scatter_plot(sdr_global_t *sdr, std::vector<std::complex<double>> &samples);
void show_test_sdr_set(sdr_global_t *sdr);
void test_bpsk_ofdm_tx(sdr_global_t *sdr);
void test_bpsk_ofdm_rx(sdr_global_t *sdr);
void test_bpsk_ofdm_demod(sdr_global_t *sdr);
void show_realtime_ofdm_window(sdr_global_t *sdr);

#endif // PLUTO_UI_H