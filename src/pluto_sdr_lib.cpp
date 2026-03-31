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
            int flags;
            long long timeNs;

            int sr = SoapySDRDevice_readStream(sdr->sdr, sdr->rxStream, rx_buffs, sdr->sdr_config.buffer_size, &flags, &timeNs, timeoutUs);
            if (sr < 0)
            {
                printf("ERROR. SoapySDRDevice_readStream.\n");
                continue;
            }
            
            if(!sdr->sdr_config.is_tx){
                // Конвертируем сырые данные в комплексные сэмплы
                for (int i = 0; i < BUFFER_SIZE; i++){
                    sdr->phy.raw_samples[i] = std::complex<double>(
                        sdr->phy.pluto_rx_buffer[i * 2] / 2048.0, 
                        sdr->phy.pluto_rx_buffer[i * 2 + 1] / 2048.0
                    );
                }
                
                // 1. Согласованный фильтр (Matched Filter)
                // Создаем SRRC фильтр
                int nsps = sdr->phy.Nsps;
                int syms = 5;
                double beta = 0.75;
                std::vector<double> filter = srrc(syms, beta, nsps, 0.0f);
                
                // Применяем согласованный фильтр к сырым сэмплам
                sdr->phy.matched_samples = convolve(sdr->phy.raw_samples, filter);
                
                // 2. Символьная синхронизация (Symbol Sync)
                // Используем Mueller-Muller clock recovery для BPSK
                sdr->phy.symb_sync_samples = clock_recovery_mueller_muller(sdr->phy.matched_samples, nsps);
                
                // 3. Частотная синхронизация (Costas Loop) для BPSK
                sdr->phy.costas_sync_samples = costas_loop_bpsk(sdr->phy.symb_sync_samples);
                
                // Для отладки - выводим размеры массивов
                static int debug_counter = 0;
                if (debug_counter++ % 100 == 0) {
                    std::cout << "Raw: " << sdr->phy.raw_samples.size() 
                              << " Matched: " << sdr->phy.matched_samples.size()
                              << " SymbSync: " << sdr->phy.symb_sync_samples.size()
                              << " Costas: " << sdr->phy.costas_sync_samples.size() << std::endl;
                }
            }

            sdr->phy.rx_timeNs = timeNs;
            last_time = timeNs;

            // Переменная для времени отправки сэмплов относительно текущего приема
            long long tx_time = timeNs + (4 * 1000 * 1000); // на 4 [мс] в будущее

            void *tx_buffs[] = {sdr->phy.pluto_tx_buffer};
            // Добавляем время, когда нужно передать блок tx_buffer
            for(size_t i = 0; i < 8; i++)
            {
                uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
                sdr->phy.pluto_tx_buffer[2 + i] = tx_time_byte << 4;
            }
            
            if(sdr->sdr_config.is_tx == true){
                if( (buffers_read % 2 == 0) ){
                    flags = SOAPY_SDR_HAS_TIME;
                    int st = SoapySDRDevice_writeStream(sdr->sdr, sdr->txStream, (const void * const*)tx_buffs, 
                                                        sdr->sdr_config.buffer_size, &flags, tx_time, timeoutUs);
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