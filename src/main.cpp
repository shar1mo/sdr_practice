#include <stdio.h> //printf
#include <stdlib.h> //free
#include <stdint.h>
#include <complex.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <thread>

#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"

#include "pluto_sdr_lib.h"
#include "ui/pluto_ui.h"



int main(int argc, char* argv[])
{
    sdr_global_t my_sdr;
    my_sdr.running = true;

    std::cout << "argc = " << argc << std::endl;

    if(argc > 2){
        my_sdr.sdr_config.name = argv[1];
        if(*argv[2] == '0'){
            my_sdr.sdr_config.is_tx = false;
            my_sdr.sdr_config.tx_carrier_freq = 700e6;
            my_sdr.sdr_config.rx_carrier_freq = 750e6;
        } else {
            my_sdr.sdr_config.is_tx = true;
            my_sdr.sdr_config.tx_carrier_freq = 750e6;
            my_sdr.sdr_config.rx_carrier_freq = 700e6;
        }
    } else {
        my_sdr.sdr_config.name = "sd";
        my_sdr.sdr_config.is_tx = false;
        my_sdr.sdr_config.tx_carrier_freq = 750e6;
        my_sdr.sdr_config.rx_carrier_freq = 700e6;
    }

    if (argc > 3) {
        my_sdr.use_ofdm = (*argv[3] == '1');
    } else {
        my_sdr.use_ofdm = false;
    }

    my_sdr.sdr_config.buffer_size = BUFFER_SIZE;
    my_sdr.sdr_config.tx_bandwidth = 2e6;
    my_sdr.sdr_config.rx_bandwidth = 2e6;
    my_sdr.sdr_config.rx_sample_rate = 4e6;
    my_sdr.sdr_config.tx_sample_rate = 4e6;
    my_sdr.sdr_config.rx_gain = 35.0;
    my_sdr.sdr_config.tx_gain = -20.0;

    my_sdr.phy.raw_samples.resize(my_sdr.sdr_config.buffer_size);
    my_sdr.phy.matched_samples.resize(my_sdr.sdr_config.buffer_size);
    my_sdr.phy.symb_sync_samples.resize(my_sdr.sdr_config.buffer_size / my_sdr.phy.Nsps);
    my_sdr.phy.costas_sync_samples.resize(my_sdr.sdr_config.buffer_size / my_sdr.phy.Nsps);

    std::cout << "vector size = " << my_sdr.phy.raw_samples.size() << std::endl;
    std::cout << "my_sdr.sdr_config.is_tx = " << my_sdr.sdr_config.is_tx << std::endl;
    std::cout << "my_sdr.use_ofdm = " << my_sdr.use_ofdm << std::endl;

    my_sdr.sdr = setup_pluto_sdr(&my_sdr.sdr_config);
    my_sdr.rxStream = setup_stream(my_sdr.sdr, &my_sdr.sdr_config, 1);
    my_sdr.txStream = setup_stream(my_sdr.sdr, &my_sdr.sdr_config, 0);

    prepare_test_tx_buffer(&my_sdr);
    test_rx_bpsk_barker13(&my_sdr);
    test_bpsk_ofdm_loopback(&my_sdr);

    if (my_sdr.use_ofdm) {
        prepare_test_tx_buffer_ofdm(&my_sdr);
    }

    std::thread gui_thread(run_gui, &my_sdr);
    std::thread sdr_thread(run_sdr, &my_sdr);

    gui_thread.join();
    sdr_thread.join();

    return EXIT_SUCCESS;
}
