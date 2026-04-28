#include <fftw3.h>
#include <stdio.h> //printf
#include <stdlib.h> //free
#include <stdint.h>
#include <complex>
#include <vector>
#include <iostream> 
#include <cstdint>
#include <numeric>
#include <algorithm>
#include <cmath>

#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>

#include "bit_generation.h"
#include "pulse_shaping.h"
#include "modulation.h"
#include "math_operations.h"
#include "freq_sync.h"
#include "time_sync.h"
#include "test_rx_samples_pam_qam4_2_barker13.h"
#include "test_rx_samples_bpsk_barker13.h"
#include "pluto_sdr_lib.h"
#include "ofdm.h"

struct SoapySDRDevice *setup_pluto_sdr(sdr_config_t *config)
{
    // Создание сущности устройства
    SoapySDRKwargs args = {};
    char buffer_size[10]; // Allocate enough space
    sprintf(buffer_size, "%d", config->buffer_size);
    SoapySDRKwargs_set(&args, "driver", "plutosdr");
    if (1) {
        SoapySDRKwargs_set(&args, "uri", config->name);
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.1");
    }
    SoapySDRKwargs_set(&args, "direct", "1");
    SoapySDRKwargs_set(&args, "timestamp_every", buffer_size);
    SoapySDRKwargs_set(&args, "loopback", "0");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

    if (sdr == NULL)
    {
        printf("SoapySDRDevice_make fail: %s\n", SoapySDRDevice_lastError());
        return NULL;
    }

    // Настраиваем устройства RX\TX
    if (SoapySDRDevice_setBandwidth(sdr, SOAPY_SDR_RX, 0, config->rx_bandwidth) != 0)
    {
        printf("setSampleRate rx fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, config->rx_sample_rate) != 0)
    {
        printf("setSampleRate rx fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, config->rx_carrier_freq, NULL) != 0)
    {
        printf("setFrequency rx fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setBandwidth(sdr, SOAPY_SDR_TX, 0, config->tx_bandwidth) != 0)
    {
        printf("setSampleRate rx fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, config->tx_sample_rate) != 0)
    {
        printf("setSampleRate tx fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, config->tx_carrier_freq, NULL) != 0)
    {
        printf("setFrequency tx fail: %s\n", SoapySDRDevice_lastError());
    }
    printf("SoapySDRDevice_getFrequency tx: %lf\n", SoapySDRDevice_getFrequency(sdr, SOAPY_SDR_TX, 0));

    return sdr;
}

struct SoapySDRStream *setup_stream(struct SoapySDRDevice *sdr, sdr_config_t *config, bool isRx)
{
    if(sdr){
        size_t channel_count = sizeof(config->channels) / sizeof(config->channels[0]);
        SoapySDRStream *stream;

        if(isRx){
            stream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, config->channels, channel_count, NULL);
            if (stream == NULL)
            {
                printf("setupStream rx fail: %s\n", SoapySDRDevice_lastError());
                SoapySDRDevice_unmake(sdr);
                return NULL;
            }
            if(SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, *config->channels, config->rx_gain) !=0 ){
                printf("setGain rx fail: %s\n", SoapySDRDevice_lastError());
            }
            
            size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, stream);
            printf("MTU - RX: %lu\n",rx_mtu);

            //activate streams
            SoapySDRDevice_activateStream(sdr, stream, 0, 0, 0); //start streaming
            
            printf("Streams are activated\n");
        } else {
            stream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, config->channels, channel_count, NULL);
            if (stream == NULL)
            {
                printf("setupStream tx fail: %s\n", SoapySDRDevice_lastError());
                SoapySDRDevice_unmake(sdr);
                return NULL;
            }
            if(SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, *config->channels, config->tx_gain) !=0 ){
                printf("setGain tx fail: %s\n", SoapySDRDevice_lastError());
            }
            size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, stream);
            printf("MTU - TX: %lu\n", tx_mtu);

            SoapySDRDevice_activateStream(sdr, stream, 0, 0, 0); //start streaming
        }
        return stream;
    } else {
        return NULL;
    }
}

void fill_test_tx_buffer(int16_t *buffer, int size)
{
    int Nsps = 10;
    std::vector<int> bit_array = generate_bit_array(size/Nsps*2);
    std::vector<std::complex<double>> modulated_array = modulate(bit_array, 2);
    std::vector<std::complex<double>> upsampled_bit_array = upsample(modulated_array, Nsps);
    std::vector<std::complex<double>> pulse_shaped = pulse_shaping(upsampled_bit_array, 0, Nsps);
    std::cout << "bit_array size = " << bit_array.size() << std::endl;
    std::cout << "modulated_array size = " << modulated_array.size() << std::endl;
    std::cout << "upsampled_bit_array size = " << upsampled_bit_array.size() << std::endl;
    std::cout << "pulse_shaped size = " << pulse_shaped.size() << std::endl;

    for (int i = 0; i < 2 * size; i+=2)
    {
        buffer[i] = int(pulse_shaped[i].real() * 2000) << 4;
        buffer[i + 1] = int(pulse_shaped[i].imag() * 2000) << 4;
        //std::cout << buffer[i] << " " << buffer[i+1] << " ";
    }
    std::cout << std::endl;

    //prepare fixed bytes in transmit buffer
    //we transmit a pattern of FFFF FFFF [TS_0]00 [TS_1]00 [TS_2]00 [TS_3]00 [TS_4]00 [TS_5]00 [TS_6]00 [TS_7]00 FFFF FFFF
    //that is a flag (FFFF FFFF) followed by the 64 bit timestamp, split into 8 bytes and packed into the lsb of each of the DAC words.
    //DAC samples are left aligned 12-bits, so each byte is left shifted into place
    for(size_t i = 0; i < 2; i++)
    {
        buffer[0 + i] = 0xffff;
        // 8 x timestamp words
        buffer[10 + i] = 0xffff;
    }
}

void prepare_test_tx_buffer_ofdm(sdr_global_t *sdr)
{
    std::vector<int> hello_sibguti = {
        0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,
        0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1,
        1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1,
        1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0,
        0
    };

    sdr->test_bpsk_ofdm.params = init_ofdm_params();

    sdr->test_bpsk_ofdm.bit_array = hello_sibguti;
    sdr->test_bpsk_ofdm.modulated_symbols = modulate(sdr->test_bpsk_ofdm.bit_array, 1);
    sdr->test_bpsk_ofdm.ofdm_tx_samples = ofdm_modulate(
        sdr->test_bpsk_ofdm.modulated_symbols,
        sdr->test_bpsk_ofdm.params
    );

    sdr->test_bpsk_ofdm.carrier_map.assign(sdr->test_bpsk_ofdm.params.fft_size, 0.0);
    for (int i = 0; i < (int)sdr->test_bpsk_ofdm.params.data_carriers.size(); i++) {
        sdr->test_bpsk_ofdm.carrier_map[sdr->test_bpsk_ofdm.params.data_carriers[i]] = 1.0;
    }
    for (int i = 0; i < (int)sdr->test_bpsk_ofdm.params.pilot_carriers.size(); i++) {
        sdr->test_bpsk_ofdm.carrier_map[sdr->test_bpsk_ofdm.params.pilot_carriers[i]] = 2.0;
    }

    int sym_len = sdr->test_bpsk_ofdm.params.fft_size + sdr->test_bpsk_ofdm.params.cp_len;
    if ((int)sdr->test_bpsk_ofdm.ofdm_tx_samples.size() >= sym_len) {
        sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.resize(sdr->test_bpsk_ofdm.params.fft_size);
        for (int i = 0; i < sdr->test_bpsk_ofdm.params.fft_size; i++) {
            sdr->test_bpsk_ofdm.ofdm_symbol_no_cp[i] =
                sdr->test_bpsk_ofdm.ofdm_tx_samples[sdr->test_bpsk_ofdm.params.cp_len + i];
        }

        sdr->test_bpsk_ofdm.fft_symbol = fft(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp);
        sdr->test_bpsk_ofdm.fft_magnitude.resize(sdr->test_bpsk_ofdm.fft_symbol.size());
        for (int i = 0; i < (int)sdr->test_bpsk_ofdm.fft_symbol.size(); i++) {
            sdr->test_bpsk_ofdm.fft_magnitude[i] = std::abs(sdr->test_bpsk_ofdm.fft_symbol[i]);
        }
    }

    // Header занимает 12 int16 слов:
    // [0..1] = FFFF FFFF
    // [2..9] = timestamp
    // [10..11] = FFFF FFFF
    // payload должен начинаться с 12, а не с 13
    int payload_start = 12;
    int k = 0;

    for (int i = payload_start; i < 2 * sdr->sdr_config.buffer_size; i += 2)
    {
        auto val = sdr->test_bpsk_ofdm.ofdm_tx_samples[
            k % sdr->test_bpsk_ofdm.ofdm_tx_samples.size()
        ];

        sdr->phy.pluto_tx_buffer[i]     = int(val.real() * 2000.0) << 4;
        sdr->phy.pluto_tx_buffer[i + 1] = int(val.imag() * 2000.0) << 4;
        k++;
    }

    for (size_t i = 0; i < 2; i++)
    {
        sdr->phy.pluto_tx_buffer[0 + i]  = 0xffff;
        sdr->phy.pluto_tx_buffer[10 + i] = 0xffff;
    }

    std::cout << "OFDM TX payload bits = " << sdr->test_bpsk_ofdm.bit_array.size() << std::endl;
    std::cout << "OFDM TX symbols = " << sdr->test_bpsk_ofdm.modulated_symbols.size() << std::endl;
    std::cout << "OFDM TX samples = " << sdr->test_bpsk_ofdm.ofdm_tx_samples.size() << std::endl;
}

static int find_ofdm_symbol_start(
    const std::vector<std::complex<double>> &samples,
    ofdm_params_t &params,
    double &best_metric
)
{
    int sym_len = params.fft_size + params.cp_len;
    if ((int)samples.size() < sym_len) {
        best_metric = 0.0;
        return 0;
    }

    best_metric = 0.0;
    int best_index = 0;

    for (int n = 0; n <= (int)samples.size() - sym_len; n++)
    {
        std::complex<double> corr = 0.0;
        double p1 = 0.0;
        double p2 = 0.0;

        for (int i = 0; i < params.cp_len; i++)
        {
            const auto &a = samples[n + i];
            const auto &b = samples[n + params.fft_size + i];
            corr += a * std::conj(b);
            p1 += std::norm(a);
            p2 += std::norm(b);
        }

        double denom = std::sqrt(p1 * p2) + 1e-12;
        double metric = std::abs(corr) / denom;

        if (metric > best_metric)
        {
            best_metric = metric;
            best_index = n;
        }
    }

    return best_index;
}

static double estimate_cfo_from_cp(
    const std::vector<std::complex<double>> &samples,
    int start,
    const ofdm_params_t &params
)
{
    std::complex<double> corr = 0.0;

    for (int i = 0; i < params.cp_len; i++)
    {
        const auto &a = samples[start + i];
        const auto &b = samples[start + params.fft_size + i];
        corr += a * std::conj(b);
    }

    double phase = std::arg(corr);

    // corr ~ exp(-j * w * fft_size)
    // => w ~= -phase / fft_size
    return -phase / (double)params.fft_size;
}

static std::vector<std::complex<double>> apply_cfo_correction(
    const std::vector<std::complex<double>> &samples,
    double cfo_rad_per_sample
)
{
    std::vector<std::complex<double>> out(samples.size());

    for (size_t n = 0; n < samples.size(); n++)
    {
        std::complex<double> rot = std::exp(
            std::complex<double>(0.0, -cfo_rad_per_sample * (double)n)
        );
        out[n] = samples[n] * rot;
    }

    return out;
}

void process_rx_ofdm_realtime(sdr_global_t *sdr)
{
    sdr->test_bpsk_ofdm.ofdm_rx_samples = sdr->phy.raw_samples;

    if (sdr->test_bpsk_ofdm.params.fft_size == 0) {
        sdr->test_bpsk_ofdm.params = init_ofdm_params();
    }

    const int fft_size = sdr->test_bpsk_ofdm.params.fft_size;
    const int cp_len   = sdr->test_bpsk_ofdm.params.cp_len;
    const int sym_len  = fft_size + cp_len;

    if ((int)sdr->phy.raw_samples.size() < sym_len) {
        sdr->test_bpsk_ofdm.rx_data_symbols.clear();
        sdr->test_bpsk_ofdm.demod_bit_array.clear();
        sdr->test_bpsk_ofdm.cfo_corrected_samples.clear();
        sdr->test_bpsk_ofdm.phase_corrected_symbol.clear();
        sdr->test_bpsk_ofdm.equalized_data_symbols.clear();
        sdr->test_bpsk_ofdm.detected_symbol_start = 0;
        sdr->test_bpsk_ofdm.detected_symbols_in_buffer = 0;
        sdr->test_bpsk_ofdm.detected_cp_metric = 0.0;
        sdr->test_bpsk_ofdm.estimated_cfo_rad_per_sample = 0.0;
        sdr->test_bpsk_ofdm.estimated_common_phase = 0.0;
        return;
    }

    // 1. Поиск начала OFDM-символа
    double best_metric = 0.0;
    int start = find_ofdm_symbol_start(
        sdr->phy.raw_samples,
        sdr->test_bpsk_ofdm.params,
        best_metric
    );

    sdr->test_bpsk_ofdm.detected_symbol_start = start;
    sdr->test_bpsk_ofdm.detected_cp_metric = best_metric;

    int symbols_in_buffer = ((int)sdr->phy.raw_samples.size() - start) / sym_len;
    sdr->test_bpsk_ofdm.detected_symbols_in_buffer = symbols_in_buffer;

    if (symbols_in_buffer <= 0) {
        sdr->test_bpsk_ofdm.rx_data_symbols.clear();
        sdr->test_bpsk_ofdm.demod_bit_array.clear();
        sdr->test_bpsk_ofdm.cfo_corrected_samples.clear();
        sdr->test_bpsk_ofdm.phase_corrected_symbol.clear();
        sdr->test_bpsk_ofdm.equalized_data_symbols.clear();
        return;
    }

    // 2. Выровненный OFDM-блок
    std::vector<std::complex<double>> aligned_ofdm_samples;
    aligned_ofdm_samples.reserve(symbols_in_buffer * sym_len);

    for (int i = 0; i < symbols_in_buffer * sym_len; i++) {
        aligned_ofdm_samples.push_back(sdr->phy.raw_samples[start + i]);
    }

    // 3. Оценка CFO по cyclic prefix
    double cfo_rad_per_sample = estimate_cfo_from_cp(
        sdr->phy.raw_samples,
        start,
        sdr->test_bpsk_ofdm.params
    );
    sdr->test_bpsk_ofdm.estimated_cfo_rad_per_sample = cfo_rad_per_sample;

    // 4. Коррекция CFO
    sdr->test_bpsk_ofdm.cfo_corrected_samples =
        apply_cfo_correction(aligned_ofdm_samples, cfo_rad_per_sample);

    // 5. Ручной realtime-demod с фазовой коррекцией по пилотам
    sdr->test_bpsk_ofdm.rx_data_symbols.clear();
    sdr->test_bpsk_ofdm.equalized_data_symbols.clear();
    sdr->test_bpsk_ofdm.phase_corrected_symbol.clear();
    sdr->test_bpsk_ofdm.estimated_common_phase = 0.0;

    for (int s = 0; s < symbols_in_buffer; s++)
    {
        int base = s * sym_len;
        if (base + sym_len > (int)sdr->test_bpsk_ofdm.cfo_corrected_samples.size()) {
            break;
        }

        std::vector<std::complex<double>> symbol_no_cp(fft_size);
        for (int i = 0; i < fft_size; i++) {
            symbol_no_cp[i] =
                sdr->test_bpsk_ofdm.cfo_corrected_samples[base + cp_len + i];
        }

        std::vector<std::complex<double>> freq = fft(symbol_no_cp);

        // Common phase correction по pilot carriers
        std::complex<double> pilot_sum = 0.0;
        for (int i = 0; i < (int)sdr->test_bpsk_ofdm.params.pilot_carriers.size(); i++) {
            int sc = sdr->test_bpsk_ofdm.params.pilot_carriers[i];
            pilot_sum += freq[sc];
        }

        double common_phase = 0.0;
        if (std::abs(pilot_sum) > 1e-12) {
            common_phase = std::arg(pilot_sum);
            std::complex<double> rot = std::exp(std::complex<double>(0.0, -common_phase));
            for (int i = 0; i < (int)freq.size(); i++) {
                freq[i] *= rot;
            }
        }

        if (s == 0) {
            sdr->test_bpsk_ofdm.ofdm_symbol_no_cp = symbol_no_cp;
            sdr->test_bpsk_ofdm.fft_symbol = freq;
            sdr->test_bpsk_ofdm.phase_corrected_symbol = freq;
            sdr->test_bpsk_ofdm.estimated_common_phase = common_phase;

            sdr->test_bpsk_ofdm.fft_magnitude.resize(freq.size());
            for (int i = 0; i < (int)freq.size(); i++) {
                sdr->test_bpsk_ofdm.fft_magnitude[i] = std::abs(freq[i]);
            }
        }

        for (int i = 0; i < (int)sdr->test_bpsk_ofdm.params.data_carriers.size(); i++) {
            int sc = sdr->test_bpsk_ofdm.params.data_carriers[i];
            auto val = freq[sc];
            sdr->test_bpsk_ofdm.rx_data_symbols.push_back(val);
            sdr->test_bpsk_ofdm.equalized_data_symbols.push_back(val);
        }
    }

    // 6. Обрезаем до ожидаемой длины
    if (sdr->test_bpsk_ofdm.rx_data_symbols.size() >
        sdr->test_bpsk_ofdm.modulated_symbols.size()) {
        sdr->test_bpsk_ofdm.rx_data_symbols.resize(
            sdr->test_bpsk_ofdm.modulated_symbols.size()
        );
    }

    sdr->test_bpsk_ofdm.demod_bit_array =
        demodulate(sdr->test_bpsk_ofdm.rx_data_symbols, 1);

    if (sdr->test_bpsk_ofdm.demod_bit_array.size() >
        sdr->test_bpsk_ofdm.bit_array.size()) {
        sdr->test_bpsk_ofdm.demod_bit_array.resize(
            sdr->test_bpsk_ofdm.bit_array.size()
        );
    }
}

void run_sdr(sdr_global_t *sdr)
{
    if (sdr->sdr)
    {
        const long timeoutUs = 400000;
        long long last_time = 0;

        int buffers_read = 0;

        while (sdr->running)
        {
            void *rx_buffs[] = {sdr->phy.pluto_rx_buffer};
            int flags = 0;
            long long timeNs = 0;

            int sr = SoapySDRDevice_readStream(
                sdr->sdr,
                sdr->rxStream,
                rx_buffs,
                sdr->sdr_config.buffer_size,
                &flags,
                &timeNs,
                timeoutUs
            );

            if (sr < 0)
            {
                printf("ERROR. SoapySDRDevice_readStream.\n");
                continue;
            }

            if (!sdr->sdr_config.is_tx)
            {
                for (int i = 0; i < BUFFER_SIZE; i++)
                {
                    sdr->phy.raw_samples[i] = std::complex<double>(
                        sdr->phy.pluto_rx_buffer[i * 2] / 2048.0,
                        sdr->phy.pluto_rx_buffer[i * 2 + 1] / 2048.0
                    );
                }

                if (sdr->use_ofdm)
                {
                    process_rx_ofdm_realtime(sdr);
                }
                else
                {
                    int nsps = sdr->phy.Nsps;
                    int syms = 5;
                    double beta = 0.75;
                    std::vector<double> filter = srrc(syms, beta, nsps, 0.0f);

                    sdr->phy.matched_samples = convolve(sdr->phy.raw_samples, filter);
                    sdr->phy.symb_sync_samples =
                        clock_recovery_mueller_muller(sdr->phy.matched_samples, nsps);
                    sdr->phy.costas_sync_samples =
                        costas_loop_bpsk(sdr->phy.symb_sync_samples);

                    static int debug_counter_bpsk = 0;
                    if (debug_counter_bpsk++ % 100 == 0)
                    {
                        std::cout
                            << "BPSK | Raw: " << sdr->phy.raw_samples.size()
                            << " Matched: " << sdr->phy.matched_samples.size()
                            << " SymbSync: " << sdr->phy.symb_sync_samples.size()
                            << " Costas: " << sdr->phy.costas_sync_samples.size()
                            << std::endl;
                    }
                }
            }

            sdr->phy.rx_timeNs = timeNs;
            last_time = timeNs;

            long long tx_time = timeNs + (4 * 1000 * 1000);

            void *tx_buffs[] = {sdr->phy.pluto_tx_buffer};

            for (size_t i = 0; i < 8; i++)
            {
                uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
                sdr->phy.pluto_tx_buffer[2 + i] = tx_time_byte << 4;
            }

            if (sdr->sdr_config.is_tx == true)
            {
                if ((buffers_read % 2) == 0)
                {
                    flags = SOAPY_SDR_HAS_TIME;

                    int st = SoapySDRDevice_writeStream(
                        sdr->sdr,
                        sdr->txStream,
                        (const void * const *)tx_buffs,
                        sdr->sdr_config.buffer_size,
                        &flags,
                        tx_time,
                        timeoutUs
                    );

                    if (st != sdr->sdr_config.buffer_size)
                    {
                        printf("TX Failed: %i\n", st);
                    }
                }

                buffers_read++;
            }
        }

        close_pluto_sdr(sdr);
    }
}

void prepare_test_tx_buffer(sdr_global_t *sdr)
{
    // 1. Код Баркера
    std::cout << "Формируем код Баркера (13 бит)" << std::endl;
    std::vector<int> barker_real = {1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1};
    std::vector<int> barker_imag = {1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1};
    std::vector<std::complex<double>> barker_complex;
    for(int i = 0; i < (int)barker_real.size(); i++){
        barker_complex.push_back(std::complex(barker_real[i] * 1.1f, barker_imag[i] * 1.1f));
        //std::cout << barker_complex[i] << " ";
    }
    // std::cout << std::endl;

    // 2. "0X00Hello from user10" в ASCII\UTF-8 - 104 бита
    std::vector<int> hello_sibguti = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 
        0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 
        1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 
        1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 
        0, 0, 0, 1, 1, 0, 0, 0, 0};

    std::cout << "text len = " << hello_sibguti.size() << std::endl;

    // 3. Модуляция QPSK
    std::vector<std::complex<double>> modulated_data = modulate(hello_sibguti, 1);

    // 4. Объединяем Код Баркера с текстом (после модуляции)
    std::vector<std::complex<double>> frame_data;
    frame_data.reserve(barker_complex.size() + modulated_data.size());
    frame_data.insert(frame_data.end(), barker_complex.begin(), barker_complex.end());
    frame_data.insert(frame_data.end(), modulated_data.begin(), modulated_data.end());
    
    std::vector<std::complex<double>> summ_vector;
    for (int i = 0; i < 1; i++)
    {
        summ_vector.insert(summ_vector.end(), frame_data.begin(), frame_data.end());
    }
    // std::cout << "frame_data = " << summ_vector.size() << std::endl;
    // for (int i = 0; i < frame_data.size();i++){
    //     std::cout << frame_data[i] << " ";
    // }
    // std::cout << std::endl;
    std::cout << "summ_vector len = " << summ_vector.size() << std::endl;
    
    // 5. Апсемплинг и Формирующий фильтр
    int nsps = sdr->phy.Nsps;
    int syms = 5;
    double beta = 0.75;
    std::vector<std::complex<double>> upsampled = upsample(summ_vector, nsps);
    std::vector<double> filter = srrc(syms, beta, nsps, 0.0f);
    sdr->test_rx_sdr.pulse_shaped = convolve(upsampled, filter);
    for (int i = 0; i < (int)sdr->test_rx_sdr.pulse_shaped.size(); i++)
    {
        sdr->test_rx_sdr.samples_to_tx.push_back({  int(sdr->test_rx_sdr.pulse_shaped[i].real() * 2000) << 4, 
                                                    int(sdr->test_rx_sdr.pulse_shaped[i].imag() * 2000) << 4});
    }
    std::cout << "pulse size = " << sdr->test_rx_sdr.pulse_shaped.size() << std::endl;

    int j = 0;
    for (int i = 13; i < 2 * sdr->sdr_config.buffer_size; i += 2)
    {
        if(j < (int)sdr->test_rx_sdr.pulse_shaped.size()){
            
            sdr->phy.pluto_tx_buffer[i] = sdr->test_rx_sdr.samples_to_tx[j].real();
            sdr->phy.pluto_tx_buffer[i + 1] = sdr->test_rx_sdr.samples_to_tx[j].imag();
            j++;
        }
        else
        {
            sdr->phy.pluto_tx_buffer[i] = int(3000);
            sdr->phy.pluto_tx_buffer[i + 1] = int(3000);
        }
    }

    //  Подготовим Флаги для Timestamp поля.
    for(size_t i = 0; i < 2; i++)
    {
        sdr->phy.pluto_tx_buffer[0 + i] = 0xffff;
        // 8 x timestamp words
        sdr->phy.pluto_tx_buffer[10 + i] = 0xffff;
    }


}

void test_rx_bpsk_barker13(sdr_global_t *sdr)
{
    std::vector<int> hello_sibguti = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 
        0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 
        1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 
        1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 
        0};
    sdr->test_bpsk_barker13.bit_array = hello_sibguti;
    int buffer_size = sdr->sdr_config.buffer_size;
    int sample_rate = sdr->sdr_config.rx_sample_rate;
    int nsps = sdr->phy.Nsps;
    
    // 1. Кладем сэмплы в буффер (по размеру буфера)
    int target_idx = 8200;
    for (int i = target_idx; i < target_idx + buffer_size; i++){
        sdr->test_bpsk_barker13.channel_samples.push_back(test_rx_samples_bpsk_barker13[i] / std::pow(2, 11));
    }

    // 2. Согласованный фильтр (SRRC)
    int syms = 5;
    double beta = 0.75;
    std::vector<double> filter = srrc(syms, beta, nsps, 0.0f);
    sdr->test_bpsk_barker13.matched_samples = convolve(sdr->test_bpsk_barker13.channel_samples, filter);

    // 3. Символьная синхронизация \ downsampling
    sdr->test_bpsk_barker13.ted_samples = clock_recovery_mueller_muller(sdr->test_bpsk_barker13.matched_samples, nsps);

    // 4. Частотная синхронизация (Costas Loop)
    sdr->test_bpsk_barker13.costas_samples = costas_loop_bpsk(sdr->test_bpsk_barker13.ted_samples);
    
    // 4.5 Нормируем к единице
    double max = 0.0f;
    sdr->test_bpsk_barker13.quantalph.resize(sdr->test_bpsk_barker13.costas_samples.size());
    for (int i = 0; i < sdr->test_bpsk_barker13.costas_samples.size(); i++) {
        if(sdr->test_bpsk_barker13.costas_samples[i].real() > max){
            max = sdr->test_bpsk_barker13.costas_samples[i].real();
        }
    }
    for (int i = 0; i < sdr->test_bpsk_barker13.costas_samples.size(); i++) {
        sdr->test_bpsk_barker13.quantalph[i] = sdr->test_bpsk_barker13.costas_samples[i] / max;
    }
    // 5. Корреляция по Баркеру
    std::vector<int> barker_real = {1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1};
    std::vector<int> barker_imag = {1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1};
    std::vector<std::complex<double>> barker_complex;
    for(int i = 0; i < (int)barker_real.size(); i++){
        barker_complex.push_back(std::complex(barker_real[i] * 1.1f, barker_imag[i] * 1.1f));
    }
    sdr->test_bpsk_barker13.barker_correlation = correlate(sdr->test_bpsk_barker13.quantalph, barker_complex);
    int barker_index = 0;
    double max_threashold = 8;
    for (int i = 0; i < sdr->test_bpsk_barker13.barker_correlation.size(); i++)
    {
        if( sdr->test_bpsk_barker13.barker_correlation[i].real() >= max_threashold || 
            sdr->test_bpsk_barker13.barker_correlation[i].imag() >= max_threashold)
        {
            barker_index = i;
            std::cout << "Barker found in " << barker_index << std::endl;
        }
    }
    
    int val = 0;
    int compare = 0;

    for (int i = barker_index + 13; i < barker_index + 13 + 176; i++)
    {
        compare = i - (barker_index + 13);
        if (sdr->test_bpsk_barker13.quantalph[i].real() > 0)
        {
            val = 1;
        }
        else {
            val = 0;
        }
        sdr->test_bpsk_barker13.demod_bit_array.push_back(val);
        
        if(compare % 8 == 0){
            std::cout << " ";
        }
        std::cout << val;
    }

    std::cout << std::endl;

    std::cout << "Transmitted Bit array size = " << sdr->test_bpsk_barker13.bit_array.size() << std::endl;
    std::cout << "Received Bit array size = " << sdr->test_bpsk_barker13.demod_bit_array.size() << std::endl;
    int success_counter = 0;
    for (int i = 0; i < sdr->test_bpsk_barker13.bit_array.size(); i++)
    {
        if(sdr->test_bpsk_barker13.bit_array[i] == sdr->test_bpsk_barker13.demod_bit_array[i]){
            success_counter++;
        }
    }
    std::cout << "Success rate = " << (success_counter / sdr->test_bpsk_barker13.demod_bit_array.size()) * 100 << "%" <<std::endl;
}

void calculate_test_set(sdr_global_t *sdr)
{
    sdr->test_set.N = sdr->test_set.Nbit / sdr->test_set.MO;
    std::cout << "sdr->test_set.N = " << sdr->test_set.N << std::endl;
    sdr->test_set.xAxis.resize(sdr->test_set.N);
    sdr->test_set.xAxis_upsampled.resize(sdr->test_set.N * sdr->test_set.symb_size); 
    std::iota(sdr->test_set.xAxis.begin(), sdr->test_set.xAxis.end(), 0);
    std::iota(sdr->test_set.xAxis_upsampled.begin(), sdr->test_set.xAxis_upsampled.end(), 0);
    
    // 2. Модуляция\манипуляция BPSK\QPSK\N_QAM и др
    sdr->test_set.modulated_array = modulate(sdr->test_set.bit_array, sdr->test_set.MO);
    for (int i = 0; i < (int)sdr->test_set.modulated_array.size(); i++){
        std::cout << sdr->test_set.modulated_array[i];
    }
    std::cout << std::endl;

    // 3. Формирование Символов (upsampling)
    sdr->test_set.upsampled_bit_array = upsample(sdr->test_set.modulated_array, sdr->test_set.symb_size);

    // 4. Pulse shaping (формирующий фильтр matched filter)
    sdr->test_set.pulse_shaped = pulse_shaping(sdr->test_set.upsampled_bit_array, 0, sdr->test_set.symb_size);
    for (int i = 0; i < (int)sdr->test_set.pulse_shaped.size(); i++){
        std::cout << sdr->test_set.pulse_shaped[i] << " "; 
    }
    std::cout << "" << std::endl;

    // 5. Channel
    // sdr->test_set.channel_samples;

    // 5. Matched filter
    std::cout << "5. Matched filter" << std::endl;
    sdr->test_set.matched_samples = pulse_shaping(sdr->test_set.pulse_shaped, 0, sdr->test_set.symb_size);
    for (int i = 0; i < (int)sdr->test_set.matched_samples.size(); i++){
        std::cout << sdr->test_set.matched_samples[i] << " "; 
    }
    std::cout << "" << std::endl;

    std::cout << "6. TED" << std::endl;
    // sdr->test_set.ted_err_idx = ted(sdr->test_set.matched_samples, sdr->test_set.symb_size);
    sdr->test_set.ted_samples = symbol_sync(sdr->test_set.matched_samples, sdr->test_set.symb_size);
    sdr->test_set.ted_indexes.resize(sdr->test_set.N * sdr->test_set.symb_size);
    for (int i = 0; i < (int)sdr->test_set.ted_err_idx.size(); i++){
        sdr->test_set.ted_indexes[sdr->test_set.ted_err_idx[i]] = 10;
        std::cout << sdr->test_set.ted_err_idx[i] << " ";
    }
    std::cout << "" << std::endl;
    for (int i = 0; i < (int)sdr->test_set.ted_samples.size(); i++){
        std::cout << sdr->test_set.ted_samples[i] << " ";
    }
    std::cout << "" << std::endl;

    std::cout << "6. Costas Loop" << std::endl;
    sdr->test_set.costas_samples = costas_loop(sdr->test_set.ted_samples);
    for (int i = 0; i < (int)sdr->test_set.costas_samples.size(); i++){
        std::cout << sdr->test_set.costas_samples[i] << " ";
    }
    std::cout << "" << std::endl;
}

void close_pluto_sdr(sdr_global_t *sdr)
{
    printf("Trying to close Pluto SDR\n");
        //stop streaming
    if(sdr->sdr || sdr->rxStream || sdr->txStream){
        SoapySDRDevice_deactivateStream(sdr->sdr, sdr->rxStream, 0, 0);
        SoapySDRDevice_deactivateStream(sdr->sdr, sdr->txStream, 0, 0);

        //shutdown the stream
        SoapySDRDevice_closeStream(sdr->sdr, sdr->rxStream);
        SoapySDRDevice_closeStream(sdr->sdr, sdr->txStream);

        //cleanup device handle
        SoapySDRDevice_unmake(sdr->sdr);
    }

    printf("Pluto SDR is closed. Streams are deactivated.\n");
}

void test_bpsk_ofdm_loopback(sdr_global_t *sdr)
{
    std::vector<int> hello_sibguti = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,
        0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1,
        1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1,
        1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0,
        0};

    sdr->test_bpsk_ofdm.bit_array = hello_sibguti;
    sdr->test_bpsk_ofdm.params = init_ofdm_params();

    sdr->test_bpsk_ofdm.modulated_symbols = modulate(sdr->test_bpsk_ofdm.bit_array, 1);
    sdr->test_bpsk_ofdm.ofdm_tx_samples = ofdm_modulate(sdr->test_bpsk_ofdm.modulated_symbols, sdr->test_bpsk_ofdm.params);
    sdr->test_bpsk_ofdm.ofdm_rx_samples = sdr->test_bpsk_ofdm.ofdm_tx_samples;
    sdr->test_bpsk_ofdm.rx_data_symbols = ofdm_demodulate(sdr->test_bpsk_ofdm.ofdm_rx_samples, sdr->test_bpsk_ofdm.params);

    if (sdr->test_bpsk_ofdm.rx_data_symbols.size() > sdr->test_bpsk_ofdm.modulated_symbols.size()) {
        sdr->test_bpsk_ofdm.rx_data_symbols.resize(sdr->test_bpsk_ofdm.modulated_symbols.size());
    }

    sdr->test_bpsk_ofdm.demod_bit_array = demodulate(sdr->test_bpsk_ofdm.rx_data_symbols, 1);

    if (sdr->test_bpsk_ofdm.demod_bit_array.size() > sdr->test_bpsk_ofdm.bit_array.size()) {
        sdr->test_bpsk_ofdm.demod_bit_array.resize(sdr->test_bpsk_ofdm.bit_array.size());
    }

    sdr->test_bpsk_ofdm.carrier_map.assign(sdr->test_bpsk_ofdm.params.fft_size, 0.0);

    for (int i = 0; i < sdr->test_bpsk_ofdm.params.data_carriers.size(); i++)
    {
        sdr->test_bpsk_ofdm.carrier_map[sdr->test_bpsk_ofdm.params.data_carriers[i]] = 1.0;
    }

    for (int i = 0; i < sdr->test_bpsk_ofdm.params.pilot_carriers.size(); i++)
    {
        sdr->test_bpsk_ofdm.carrier_map[sdr->test_bpsk_ofdm.params.pilot_carriers[i]] = 2.0;
    }

    int sym_len = sdr->test_bpsk_ofdm.params.fft_size + sdr->test_bpsk_ofdm.params.cp_len;
    if ((int)sdr->test_bpsk_ofdm.ofdm_rx_samples.size() >= sym_len) {
        sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.resize(sdr->test_bpsk_ofdm.params.fft_size);
        for (int i = 0; i < sdr->test_bpsk_ofdm.params.fft_size; i++) {
            sdr->test_bpsk_ofdm.ofdm_symbol_no_cp[i] = sdr->test_bpsk_ofdm.ofdm_rx_samples[sdr->test_bpsk_ofdm.params.cp_len + i];
        }

        sdr->test_bpsk_ofdm.fft_symbol = fft(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp);
        sdr->test_bpsk_ofdm.fft_magnitude.resize(sdr->test_bpsk_ofdm.fft_symbol.size());

        for (int i = 0; i < sdr->test_bpsk_ofdm.fft_symbol.size(); i++)
        {
            sdr->test_bpsk_ofdm.fft_magnitude[i] = std::abs(sdr->test_bpsk_ofdm.fft_symbol[i]);
        }
    }

    std::cout << "OFDM BPSK TX bits = " << sdr->test_bpsk_ofdm.bit_array.size() << std::endl;
    std::cout << "OFDM BPSK RX bits = " << sdr->test_bpsk_ofdm.demod_bit_array.size() << std::endl;

    int success_counter = 0;
    int compare_size = std::min(sdr->test_bpsk_ofdm.bit_array.size(), sdr->test_bpsk_ofdm.demod_bit_array.size());
    for (int i = 0; i < compare_size; i++)
    {
        if (sdr->test_bpsk_ofdm.bit_array[i] == sdr->test_bpsk_ofdm.demod_bit_array[i]) {
            success_counter++;
        }
    }

    if (compare_size > 0) {
        std::cout << "OFDM BPSK Success rate = " << (success_counter * 100.0 / compare_size) << "%" << std::endl;
    }
}