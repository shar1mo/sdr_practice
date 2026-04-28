#include <vector>
#include <iostream>
#include <numeric>
#include <mutex>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"
#include "imgui_internal.h"

#include "bit_generation.h"
#include "pulse_shaping.h"
#include "modulation.h"
#include "math_operations.h"
#include "pluto_sdr_lib.h"
#include "ui/pluto_ui.h"
#include "ofdm.h"

#include "test_rx_samples_bpsk_barker13.h"

// Мьютекс для защиты данных при многопоточном доступе
std::mutex data_mutex;

void test_header(const char* label, void(*demo)(sdr_global_t *, test_set_t &),sdr_global_t *sdr, test_set_t &test_set) {
    if (ImGui::TreeNodeEx(label)) {
        demo(sdr, test_set);
        ImGui::TreePop();
    }
}


void run_gui(sdr_global_t *sdr)
{
    if(!sdr->sdr_config.is_tx){
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
        SDL_Window* window = SDL_CreateWindow(
            "Backend start", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        SDL_GLContext gl_context = SDL_GL_CreateContext(window);

        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL3_Init("#version 330");

        while (sdr->running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT) {
                    sdr->running = false;
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            ImGuiID dockspace_id = ImGui::GetID("My Dockspace");
            ImGuiViewport* viewport = ImGui::GetMainViewport();

            if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
            {
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
                ImGuiID dock_id_left = 0;
                ImGuiID dock_id_main = dockspace_id;
                ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.20f, &dock_id_left, &dock_id_main);
                ImGuiID dock_id_left_top = 0;
                ImGuiID dock_id_left_bottom = 0;
                ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.50f, &dock_id_left_top, &dock_id_left_bottom);
                ImGui::DockBuilderDockWindow("Main", dock_id_main);
                ImGui::DockBuilderDockWindow("Properties", dock_id_left_top);
                ImGui::DockBuilderDockWindow("Scene", dock_id_left_bottom);
                ImGui::DockBuilderFinish(dockspace_id);
            }
            ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

            show_global_menu_bar(sdr);
            show_properties_window(sdr);
            show_main_window(sdr);
            

            ImGui::Render();
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

void show_global_menu_bar(sdr_global_t *sdr)
{
    (void)sdr;
    if(ImGui::BeginMainMenuBar()){
        if (ImGui::BeginMenu("File"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}


void show_properties_window(sdr_global_t *sdr)
{
    (void)sdr;
    static int counter = 0;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::Begin("Properties");
    
    ImGui::Separator();
    ImGui::Text("Signal Processing Status:");
    ImGui::Text("Raw samples: %zu", sdr->phy.raw_samples.size());
    ImGui::Text("Matched samples: %zu", sdr->phy.matched_samples.size());
    ImGui::Text("Symbol sync samples: %zu", sdr->phy.symb_sync_samples.size());
    ImGui::Text("Costas samples: %zu", sdr->phy.costas_sync_samples.size());
    
    ImGui::End();
}

void show_main_window(sdr_global_t *sdr)
{
    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginTabBar("Main")) {
        if (ImGui::BeginTabItem("Real Time I/Q samples")) {
            
            if (!sdr->sdr_config.is_tx) {
                
                if (!sdr->phy.raw_samples.empty()) {
                    if (ImPlot::BeginPlot("Raw I/Q Samples (Time Domain)", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("Sample Index", "Amplitude");
                        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
                        std::vector<double> i_vals, q_vals;
                        size_t plot_size = std::min(sdr->phy.raw_samples.size(), (size_t)1000);
                        i_vals.reserve(plot_size);
                        q_vals.reserve(plot_size);
                        for (size_t i = 0; i < plot_size; i++) {
                            i_vals.push_back(sdr->phy.raw_samples[i].real());
                            q_vals.push_back(sdr->phy.raw_samples[i].imag());
                        }
                        ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
                        ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
                        ImPlot::EndPlot();
                    }
                }

                if (!sdr->phy.raw_samples.empty()) {
                    if (ImPlot::BeginPlot("Raw Samples Constellation", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("I","Q");
                        ImPlot::SetupAxisLimits(ImAxis_X1, -2.0, 2.0);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
                        std::vector<double> i_vals, q_vals;
                        i_vals.reserve(sdr->phy.raw_samples.size());
                        q_vals.reserve(sdr->phy.raw_samples.size());
                        for (size_t i = 0; i < sdr->phy.raw_samples.size(); i++) {
                            i_vals.push_back(sdr->phy.raw_samples[i].real());
                            q_vals.push_back(sdr->phy.raw_samples[i].imag());
                        }
                        ImPlot::PlotScatter("Signal", i_vals.data(), q_vals.data(), i_vals.size());
                        ImPlot::EndPlot();
                    }
                }

                if (!sdr->phy.matched_samples.empty()) {
                    if (ImPlot::BeginPlot("Matched Filter Output (Time Domain)", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("Sample Index", "Amplitude");
                        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
                        std::vector<double> i_vals, q_vals;
                        size_t plot_size = std::min(sdr->phy.matched_samples.size(), (size_t)1000);
                        i_vals.reserve(plot_size);
                        q_vals.reserve(plot_size);
                        for (size_t i = 0; i < plot_size; i++) {
                            i_vals.push_back(sdr->phy.matched_samples[i].real());
                            q_vals.push_back(sdr->phy.matched_samples[i].imag());
                        }
                        ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
                        ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
                        ImPlot::EndPlot();
                    }
                }

                if (!sdr->phy.matched_samples.empty()) {
                    if (ImPlot::BeginPlot("Matched Filter Constellation", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::SetupAxisLimits(ImAxis_X1, -2.0, 2.0);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
                        std::vector<double> i_vals, q_vals;
                        i_vals.reserve(sdr->phy.matched_samples.size());
                        q_vals.reserve(sdr->phy.matched_samples.size());
                        for (size_t i = 0; i < sdr->phy.matched_samples.size(); i++) {
                            i_vals.push_back(sdr->phy.matched_samples[i].real());
                            q_vals.push_back(sdr->phy.matched_samples[i].imag());
                        }
                        ImPlot::PlotScatter("Signal", i_vals.data(), q_vals.data(), i_vals.size());
                        ImPlot::EndPlot();
                    }
                }

                if (!sdr->phy.symb_sync_samples.empty()) {
                    if (ImPlot::BeginPlot("Symbol Sync Output (Time Domain)", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("Sample Index", "Amplitude");
                        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
                        std::vector<double> i_vals, q_vals;
                        i_vals.reserve(sdr->phy.symb_sync_samples.size());
                        q_vals.reserve(sdr->phy.symb_sync_samples.size());
                        for (size_t i = 0; i < sdr->phy.symb_sync_samples.size(); i++) {
                            i_vals.push_back(sdr->phy.symb_sync_samples[i].real());
                            q_vals.push_back(sdr->phy.symb_sync_samples[i].imag());
                        }
                        ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
                        ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
                        ImPlot::EndPlot();
                    }
                }

                if (!sdr->phy.symb_sync_samples.empty()) {
                    if (ImPlot::BeginPlot("Symbol Sync Constellation", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::SetupAxisLimits(ImAxis_X1, -2.0, 2.0);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
                        std::vector<double> i_vals, q_vals;
                        i_vals.reserve(sdr->phy.symb_sync_samples.size());
                        q_vals.reserve(sdr->phy.symb_sync_samples.size());
                        for (size_t i = 0; i < sdr->phy.symb_sync_samples.size(); i++) {
                            i_vals.push_back(sdr->phy.symb_sync_samples[i].real());
                            q_vals.push_back(sdr->phy.symb_sync_samples[i].imag());
                        }
                        ImPlot::PlotScatter("Signal", i_vals.data(), q_vals.data(), i_vals.size());
                        ImPlot::EndPlot();
                    }
                }

                if (!sdr->phy.costas_sync_samples.empty()) {
                    if (ImPlot::BeginPlot("Costas Loop Output (Time Domain)", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("Sample Index", "Amplitude");
                        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
                        std::vector<double> i_vals, q_vals;
                        i_vals.reserve(sdr->phy.costas_sync_samples.size());
                        q_vals.reserve(sdr->phy.costas_sync_samples.size());
                        for (size_t i = 0; i < sdr->phy.costas_sync_samples.size(); i++) {
                            i_vals.push_back(sdr->phy.costas_sync_samples[i].real());
                            q_vals.push_back(sdr->phy.costas_sync_samples[i].imag());
                        }
                        ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
                        ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
                        ImPlot::EndPlot();
                    }
                }

                if (!sdr->phy.costas_sync_samples.empty()) {
                    if (ImPlot::BeginPlot("Costas Loop Constellation", ImVec2(-1, 250))) {
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::SetupAxisLimits(ImAxis_X1, -2.0, 2.0);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
                        std::vector<double> i_vals, q_vals;
                        i_vals.reserve(sdr->phy.costas_sync_samples.size());
                        q_vals.reserve(sdr->phy.costas_sync_samples.size());
                        for (size_t i = 0; i < sdr->phy.costas_sync_samples.size(); i++) {
                            i_vals.push_back(sdr->phy.costas_sync_samples[i].real());
                            q_vals.push_back(sdr->phy.costas_sync_samples[i].imag());
                        }
                        ImPlot::PlotScatter("Signal", i_vals.data(), q_vals.data(), i_vals.size());
                        ImPlot::EndPlot();
                    }
                }
            } else {
                ImGui::Text("TX Mode - No signal processing available");
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Test: BPSK, Barker13")) {
            test_header("RX from SDR", test_rx_from_sdr, sdr, sdr->test_bpsk_barker13);
            test_header("Matched Filter", test_rx_from_sdr_matched_filter, sdr, sdr->test_bpsk_barker13);
            test_header("Symbol Sync", test_rx_from_sdr_symbol_sync, sdr, sdr->test_bpsk_barker13);
            test_header("Frequency Sync", test_rx_from_sdr_freq_sync, sdr, sdr->test_bpsk_barker13);
            test_header("Barker Code Correlation", test_rx_from_sdr_barker_corr, sdr, sdr->test_bpsk_barker13);
            test_header("Demodulation", test_rx_from_sdr_barker_demodulation, sdr, sdr->test_bpsk_barker13);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Test: BPSK OFDM")) {
            if (ImGui::TreeNodeEx("TX")) {
                test_bpsk_ofdm_tx(sdr);
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("RX")) {
                test_bpsk_ofdm_rx(sdr);
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Demodulation")) {
                test_bpsk_ofdm_demod(sdr);
                ImGui::TreePop();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Real Time OFDM")) {
            if (!sdr->sdr_config.is_tx) {
                show_realtime_ofdm_window(sdr);
            } else {
                ImGui::Text("TX Mode - OFDM realtime view is available on RX side");
            }
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void show_tx_data(sdr_global_t *sdr)
{
    static int rows = 2;
    static int cols = 1;
    static float rratios[] = {5, 5};
    static float cratios[] = {5, 5};
    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("Pulse Shaped")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_rx_sdr.pulse_shaped.size());
            q_vals.reserve(sdr->test_rx_sdr.pulse_shaped.size());
            for (size_t i = 0; i < sdr->test_rx_sdr.pulse_shaped.size(); i++) {
                i_vals.push_back(sdr->test_rx_sdr.pulse_shaped[i].real());
                q_vals.push_back(sdr->test_rx_sdr.pulse_shaped[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }
                    
        if (ImPlot::BeginPlot("Samples to TX")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(sdr->test_rx_sdr.samples_to_tx.size());
            q_vals2.reserve(sdr->test_rx_sdr.samples_to_tx.size());
            for (size_t i = 0; i < sdr->test_rx_sdr.samples_to_tx.size(); i++) {
                i_vals2.push_back(sdr->test_rx_sdr.samples_to_tx[i].real());
                q_vals2.push_back(sdr->test_rx_sdr.samples_to_tx[i].imag());
            }
            ImPlot::PlotLine("I", i_vals2.data(), i_vals2.size());
            ImPlot::PlotLine("Q", q_vals2.data(), q_vals2.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
}

void test_rx_from_sdr_barker_demodulation(sdr_global_t *sdr,  test_set_t &test_set)
{
    (void)sdr;
    ImGui::Text("TX bits: ");
    for (size_t i = 0; i < test_set.bit_array.size(); i++){
        ImGui::Text("%d", test_set.bit_array[i]);
        ImGui::SameLine();
    }
    
    ImGui::Text(" ");

    ImGui::Text("RX (demodulated) bits: ");
    for (size_t i = 0; i < test_set.demod_bit_array.size(); i++){
        ImGui::Text("%d", test_set.demod_bit_array[i]);
        ImGui::SameLine();
    }
}

void test_rx_from_sdr(sdr_global_t *sdr, test_set_t &test_set)
{
    (void)sdr;
    static int rows = 3;
    static int cols = 1;
    static float rratios[] = {5, 5, 5,};
    static float cratios[] = {5, 5, 5};
    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("Several Buffers from RX SDR")) {
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(test_rx_samples_bpsk_barker13.size());
            q_vals.reserve(test_rx_samples_bpsk_barker13.size());
            for (size_t i = 0; i < test_rx_samples_bpsk_barker13.size(); i++) {
                i_vals.push_back(test_rx_samples_bpsk_barker13[i].real());
                q_vals.push_back(test_rx_samples_bpsk_barker13[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("1 Buffer size = 1920 [samples]")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(test_set.channel_samples.size());
            q_vals2.reserve(test_set.channel_samples.size());
            for (size_t i = 0; i < test_set.channel_samples.size(); i++) {
                i_vals2.push_back(test_set.channel_samples[i].real());
                q_vals2.push_back(test_set.channel_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals2.data(), i_vals2.size());
            ImPlot::PlotLine("Q", q_vals2.data(), q_vals2.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("1 Buffer size = 1920 [samples] Constellation")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals3, q_vals3;
            i_vals3.reserve(test_set.channel_samples.size());
            q_vals3.reserve(test_set.channel_samples.size());
            for (size_t i = 0; i < test_set.channel_samples.size(); i++) {
                i_vals3.push_back(test_set.channel_samples[i].real());
                q_vals3.push_back(test_set.channel_samples[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals3.data(), q_vals3.data(), i_vals3.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
    
}

void test_rx_from_sdr_matched_filter(sdr_global_t *sdr, test_set_t &test_set)
{
    (void)sdr;
    static int rows = 2;
    static int cols = 1;
    static float rratios[] = {5, 5};
    static float cratios[] = {5, 5};
    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("Matched filter")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(test_set.matched_samples.size());
            q_vals.reserve(test_set.matched_samples.size());
            for (size_t i = 0; i < test_set.matched_samples.size(); i++) {
                i_vals.push_back(test_set.matched_samples[i].real());
                q_vals.push_back(test_set.matched_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }
        
        if (ImPlot::BeginPlot("Matched filter Constellation")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(test_set.matched_samples.size());
            q_vals2.reserve(test_set.matched_samples.size());
            for (size_t i = 0; i < test_set.matched_samples.size(); i++) {
                i_vals2.push_back(test_set.matched_samples[i].real());
                q_vals2.push_back(test_set.matched_samples[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals2.data(), q_vals2.data(), i_vals2.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }

}

void test_rx_from_sdr_coarse_freq_sync(sdr_global_t *sdr)
{
    static int rows = 5;
    static int cols = 1;
    static float rratios[] = {5, 5, 5, 5, 5};
    static float cratios[] = {5, 5, 5, 5, 5};
    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {
        
        if (ImPlot::BeginPlot("FFT Out")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_rx_sdr.fft_out_samples.size());
            q_vals.reserve(sdr->test_rx_sdr.fft_out_samples.size());
            for (size_t i = 0; i < sdr->test_rx_sdr.fft_out_samples.size(); i++) {
                i_vals.push_back(sdr->test_rx_sdr.fft_out_samples[i].real());
                q_vals.push_back(sdr->test_rx_sdr.fft_out_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("FFT abs()")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            ImPlot::PlotLine("Abs(fftshift(fft(samples)))", sdr->test_rx_sdr.fft_out_abs.data(), 
                             sdr->test_rx_sdr.fft_out_abs.size());
            ImPlot::EndPlot();
        }
        
        if (ImPlot::BeginPlot("Coarse Freq Constellation")) {
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(sdr->test_rx_sdr.coarsed_samples.size());
            q_vals2.reserve(sdr->test_rx_sdr.coarsed_samples.size());
            for (size_t i = 0; i < sdr->test_rx_sdr.coarsed_samples.size(); i++) {
                i_vals2.push_back(sdr->test_rx_sdr.coarsed_samples[i].real());
                q_vals2.push_back(sdr->test_rx_sdr.coarsed_samples[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals2.data(), q_vals2.data(), i_vals2.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Coarse Freq Time")) {
            std::vector<double> i_vals3, q_vals3;
            i_vals3.reserve(sdr->test_rx_sdr.coarsed_samples.size());
            q_vals3.reserve(sdr->test_rx_sdr.coarsed_samples.size());
            for (size_t i = 0; i < sdr->test_rx_sdr.coarsed_samples.size(); i++) {
                i_vals3.push_back(sdr->test_rx_sdr.coarsed_samples[i].real());
                q_vals3.push_back(sdr->test_rx_sdr.coarsed_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals3.data(), i_vals3.size());
            ImPlot::PlotLine("Q", q_vals3.data(), q_vals3.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Baseband shift Constellation")) {
            std::vector<double> i_vals4, q_vals4;
            i_vals4.reserve(sdr->test_rx_sdr.shifted_before_cl.size());
            q_vals4.reserve(sdr->test_rx_sdr.shifted_before_cl.size());
            for (size_t i = 0; i < sdr->test_rx_sdr.shifted_before_cl.size(); i++) {
                i_vals4.push_back(sdr->test_rx_sdr.shifted_before_cl[i].real());
                q_vals4.push_back(sdr->test_rx_sdr.shifted_before_cl[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals4.data(), q_vals4.data(), i_vals4.size());
            ImPlot::EndPlot();
        }
        
        ImPlot::EndSubplots();
    }
}

void test_rx_from_sdr_symbol_sync(sdr_global_t *sdr, test_set_t &test_set)
{
    (void)sdr;
    static int rows = 2;
    static int cols = 1;
    static float rratios[] = {5, 5};
    static float cratios[] = {5, 5};

    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("TED samples")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(test_set.ted_samples.size());
            q_vals.reserve(test_set.ted_samples.size());
            for (size_t i = 0; i < test_set.ted_samples.size(); i++) {
                i_vals.push_back(test_set.ted_samples[i].real());
                q_vals.push_back(test_set.ted_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("TED samples Constellation")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(test_set.ted_samples.size());
            q_vals2.reserve(test_set.ted_samples.size());
            for (size_t i = 0; i < test_set.ted_samples.size(); i++) {
                i_vals2.push_back(test_set.ted_samples[i].real());
                q_vals2.push_back(test_set.ted_samples[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals2.data(), q_vals2.data(), i_vals2.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
        
    }

}

void test_rx_from_sdr_freq_sync(sdr_global_t *sdr,  test_set_t &test_set)
{
    (void)sdr;
    static int rows = 2;
    static int cols = 1;
    static float rratios[] = {5, 5};
    static float cratios[] = {5, 5};

    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("Costas Loop Constellation")) {
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(test_set.costas_samples.size());
            q_vals.reserve(test_set.costas_samples.size());
            for (size_t i = 0; i < test_set.costas_samples.size(); i++) {
                i_vals.push_back(test_set.costas_samples[i].real());
                q_vals.push_back(test_set.costas_samples[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals.data(), q_vals.data(), i_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Costas Loop Time")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(test_set.costas_samples.size());
            q_vals2.reserve(test_set.costas_samples.size());
            for (size_t i = 0; i < test_set.costas_samples.size(); i++) {
                i_vals2.push_back(test_set.costas_samples[i].real());
                q_vals2.push_back(test_set.costas_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals2.data(), i_vals2.size());
            ImPlot::PlotLine("Q", q_vals2.data(), q_vals2.size());
            ImPlot::EndPlot();
        }
        
        ImPlot::EndSubplots();
    }

}

void test_pulse_shaping(sdr_global_t *sdr)
{
    test_srrc(sdr);
    test_hamming(sdr);
    test_sinc(sdr);
}

void test_rx_from_sdr_barker_corr(sdr_global_t *sdr,  test_set_t &test_set)
{
    (void)sdr;
    static int rows = 2;
    static int cols = 1;
    static float rratios[] = {5, 5};
    static float cratios[] = {5, 5};

    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("Нормировка по единице")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(test_set.quantalph.size());
            q_vals.reserve(test_set.quantalph.size());
            for (size_t i = 0; i < test_set.quantalph.size(); i++) {
                i_vals.push_back(test_set.quantalph[i].real());
                q_vals.push_back(test_set.quantalph[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Корреляция по Баркеру (13 бит)")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(test_set.barker_correlation.size());
            q_vals2.reserve(test_set.barker_correlation.size());
            for (size_t i = 0; i < test_set.barker_correlation.size(); i++) {
                i_vals2.push_back(test_set.barker_correlation[i].real());
                q_vals2.push_back(test_set.barker_correlation[i].imag());
            }
            ImPlot::PlotLine("I", i_vals2.data(), i_vals2.size());
            ImPlot::PlotLine("Q", q_vals2.data(), q_vals2.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
}

void test_sinc(sdr_global_t *sdr)
{
    (void)sdr;
    std::vector<double> x = linspace(-4, 4, 41);
    std::vector<double> val = sinc(x);
    if (ImPlot::BeginPlot("sinc()")) {
        ImPlot::PlotLine("Sinc", val.data(), val.size());
        ImPlot::EndPlot();
    }
}

void test_srrc(sdr_global_t *sdr)
{
    (void)sdr;
    if (ImPlot::BeginPlot("1. SRRC")) {
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0);
        ImPlot::SetupAxisLimits(ImAxis_X1, 30.0, 70.0);
        int over_samp_rate = 10;
        int syms = 5;
        double beta[5] = {0, 0.25, 0.5, 0.75, 1};
        std::vector<double> s1 = srrc(syms, beta[0], over_samp_rate, 0);
        std::vector<double> s2 = srrc(syms, beta[1], over_samp_rate, 0);
        std::vector<double> s3 = srrc(syms, beta[2], over_samp_rate, 0);
        std::vector<double> s4 = srrc(syms, beta[3], over_samp_rate, 0);
        std::vector<double> s5 = srrc(syms, beta[4], over_samp_rate, 0);
        ImPlot::PlotLine("beta = 0",s1.data(),s1.size());
        ImPlot::PlotLine("beta = 0.25",s2.data(),s2.size());
        ImPlot::PlotLine("beta = 0.50",s3.data(),s3.size());
        ImPlot::PlotLine("beta = 0.75",s4.data(),s4.size());
        ImPlot::PlotLine("beta = 1",s5.data(),s5.size());
        ImPlot::EndPlot();
    }
}

void test_hamming(sdr_global_t *sdr)
{
    (void)sdr;
    if (ImPlot::BeginPlot("Hamming")) {
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0);
        int M = 40;
        std::vector<double> s10 = hamming(M);
        ImPlot::PlotLine("Hamming",s10.data(),s10.size());
        ImPlot::EndPlot();
    }
}

void show_iq_scatter_plot(sdr_global_t *sdr, std::vector< std::complex<double> > &samples)
{
    (void)sdr;
    if (!samples.empty()) {
        if (ImPlot::BeginPlot("ConstellationPlot")) {
            ImPlot::SetupAxes("I","Q");
            ImPlot::SetupAxisLimits(ImAxis_X1, -3000.0, 3000.0);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -3000.0, 3000.0);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(samples.size());
            q_vals.reserve(samples.size());
            for (size_t i = 0; i < samples.size(); i++) {
                i_vals.push_back(samples[i].real());
                q_vals.push_back(samples[i].imag());
            }
            ImPlot::PlotScatter("Signal", i_vals.data(), q_vals.data(), i_vals.size());
            ImPlot::EndPlot();
        }
    }
}

void show_test_sdr_set(sdr_global_t *sdr)
{
    static int rows = 7;
    static int cols = 1;
    static float rratios[] = {5, 5, 5, 5, 5, 5, 5};
    static float cratios[] = {5, 5, 5, 5, 5, 5, 5};
    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_ShareItems | ImPlotSubplotFlags_NoLegend;
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("1. Generate bit array", ImVec2(), ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
            std::vector<double> xAxis_double(sdr->test_set.xAxis.begin(), sdr->test_set.xAxis.end());
            std::vector<double> bit_array_double(sdr->test_set.bit_array.begin(), 
                                                  sdr->test_set.bit_array.end());
            ImPlot::PlotStems("Mouse Y", xAxis_double.data(), 
                              bit_array_double.data(), 
                              sdr->test_set.N);
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("2. Modulated Samples", ImVec2(), ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_set.modulated_array.size());
            q_vals.reserve(sdr->test_set.modulated_array.size());
            for (size_t i = 0; i < sdr->test_set.modulated_array.size(); i++) {
                i_vals.push_back(sdr->test_set.modulated_array[i].real());
                q_vals.push_back(sdr->test_set.modulated_array[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("2. Modulated Constellation", ImVec2(), ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
            ImPlot::SetupAxisLimits(ImAxis_X1, -2.0, 2.0);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(sdr->test_set.modulated_array.size());
            q_vals2.reserve(sdr->test_set.modulated_array.size());
            for (size_t i = 0; i < sdr->test_set.modulated_array.size(); i++) {
                i_vals2.push_back(sdr->test_set.modulated_array[i].real());
                q_vals2.push_back(sdr->test_set.modulated_array[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals2.data(), q_vals2.data(), i_vals2.size());
            ImPlot::EndPlot();
        }
    
        if (ImPlot::BeginPlot("3. Upsampling", ImVec2(), ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
            std::vector<double> i_vals3, q_vals3;
            i_vals3.reserve(sdr->test_set.upsampled_bit_array.size());
            q_vals3.reserve(sdr->test_set.upsampled_bit_array.size());
            for (size_t i = 0; i < sdr->test_set.upsampled_bit_array.size(); i++) {
                i_vals3.push_back(sdr->test_set.upsampled_bit_array[i].real());
                q_vals3.push_back(sdr->test_set.upsampled_bit_array[i].imag());
            }
            ImPlot::PlotLine("I", i_vals3.data(), i_vals3.size());
            ImPlot::PlotLine("Q", q_vals3.data(), q_vals3.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("4. Pulse shaping", ImVec2(), ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
            std::vector<double> i_vals4;
            i_vals4.reserve(sdr->test_set.pulse_shaped.size());
            for (size_t i = 0; i < sdr->test_set.pulse_shaped.size(); i++) {
                i_vals4.push_back(sdr->test_set.pulse_shaped[i].real());
            }
            ImPlot::PlotLine("I", i_vals4.data(), i_vals4.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("6. TED", ImVec2(), ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisLimits(ImAxis_Y1, -11.0, 11.0);
            std::vector<double> i_vals5, q_vals5;
            i_vals5.reserve(sdr->test_set.matched_samples.size());
            q_vals5.reserve(sdr->test_set.matched_samples.size());
            for (size_t i = 0; i < sdr->test_set.matched_samples.size(); i++) {
                i_vals5.push_back(sdr->test_set.matched_samples[i].real());
                q_vals5.push_back(sdr->test_set.matched_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals5.data(), i_vals5.size());
            ImPlot::PlotLine("Q", q_vals5.data(), q_vals5.size());
            std::vector<double> xAxis_upsampled_double(sdr->test_set.xAxis_upsampled.begin(), 
                                                        sdr->test_set.xAxis_upsampled.end());
            std::vector<double> ted_indexes_double(sdr->test_set.ted_indexes.begin(), 
                                                    sdr->test_set.ted_indexes.end());
            ImPlot::PlotStems("Stems 2", xAxis_upsampled_double.data(),
                              ted_indexes_double.data(),
                              sdr->test_set.ted_indexes.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("7. Costas Loop Constellation", ImVec2(), ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxisLimits(ImAxis_Y1, -2.0, 2.0);
            ImPlot::SetupAxisLimits(ImAxis_X1, -2.0, 2.0);
            std::vector<double> i_vals6, q_vals6;
            i_vals6.reserve(sdr->test_set.costas_samples.size());
            q_vals6.reserve(sdr->test_set.costas_samples.size());
            for (size_t i = 0; i < sdr->test_set.costas_samples.size(); i++) {
                i_vals6.push_back(sdr->test_set.costas_samples[i].real());
                q_vals6.push_back(sdr->test_set.costas_samples[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals6.data(), q_vals6.data(), i_vals6.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
}

void test_bpsk_ofdm_tx(sdr_global_t *sdr)
{
    static int rows = 5;
    static int cols = 1;
    static float rratios[] = {5, 5, 5, 5, 5};
    static float cratios[] = {5, 5, 5, 5, 5};

    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;

    if (ImPlot::BeginSubplots("OFDM TX", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("BPSK Symbols")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_bpsk_ofdm.modulated_symbols.size());
            q_vals.reserve(sdr->test_bpsk_ofdm.modulated_symbols.size());
            for (size_t i = 0; i < sdr->test_bpsk_ofdm.modulated_symbols.size(); i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.modulated_symbols[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.modulated_symbols[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("BPSK Symbols Constellation")) {
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(sdr->test_bpsk_ofdm.modulated_symbols.size());
            q_vals2.reserve(sdr->test_bpsk_ofdm.modulated_symbols.size());
            for (size_t i = 0; i < sdr->test_bpsk_ofdm.modulated_symbols.size(); i++) {
                i_vals2.push_back(sdr->test_bpsk_ofdm.modulated_symbols[i].real());
                q_vals2.push_back(sdr->test_bpsk_ofdm.modulated_symbols[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals2.data(), q_vals2.data(), i_vals2.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Carrier Map")) {
            ImPlot::SetupAxes("Subcarrier Index", "Type");
            ImPlot::SetupAxisLimits(ImAxis_Y1, -0.2, 2.2, ImGuiCond_Always);
            ImPlot::PlotStems("Carrier Type", sdr->test_bpsk_ofdm.carrier_map.data(), sdr->test_bpsk_ofdm.carrier_map.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("OFDM TX Samples")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals3, q_vals3;
            i_vals3.reserve(sdr->test_bpsk_ofdm.ofdm_tx_samples.size());
            q_vals3.reserve(sdr->test_bpsk_ofdm.ofdm_tx_samples.size());
            for (size_t i = 0; i < sdr->test_bpsk_ofdm.ofdm_tx_samples.size(); i++) {
                i_vals3.push_back(sdr->test_bpsk_ofdm.ofdm_tx_samples[i].real());
                q_vals3.push_back(sdr->test_bpsk_ofdm.ofdm_tx_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals3.data(), i_vals3.size());
            ImPlot::PlotLine("Q", q_vals3.data(), q_vals3.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("OFDM Symbol Without CP")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals4, q_vals4;
            i_vals4.reserve(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.size());
            q_vals4.reserve(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.size());
            for (size_t i = 0; i < sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.size(); i++) {
                i_vals4.push_back(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp[i].real());
                q_vals4.push_back(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp[i].imag());
            }
            ImPlot::PlotLine("I", i_vals4.data(), i_vals4.size());
            ImPlot::PlotLine("Q", q_vals4.data(), q_vals4.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
}

void test_bpsk_ofdm_rx(sdr_global_t *sdr)
{
    static int rows = 4;
    static int cols = 1;
    static float rratios[] = {5, 5, 5, 5};
    static float cratios[] = {5, 5, 5, 5};

    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;

    if (ImPlot::BeginSubplots("OFDM RX", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("OFDM RX Samples")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_bpsk_ofdm.ofdm_rx_samples.size());
            q_vals.reserve(sdr->test_bpsk_ofdm.ofdm_rx_samples.size());
            for (size_t i = 0; i < sdr->test_bpsk_ofdm.ofdm_rx_samples.size(); i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.ofdm_rx_samples[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.ofdm_rx_samples[i].imag());
            }
            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("FFT Symbol")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);
            std::vector<double> i_vals2, q_vals2;
            i_vals2.reserve(sdr->test_bpsk_ofdm.fft_symbol.size());
            q_vals2.reserve(sdr->test_bpsk_ofdm.fft_symbol.size());
            for (size_t i = 0; i < sdr->test_bpsk_ofdm.fft_symbol.size(); i++) {
                i_vals2.push_back(sdr->test_bpsk_ofdm.fft_symbol[i].real());
                q_vals2.push_back(sdr->test_bpsk_ofdm.fft_symbol[i].imag());
            }
            ImPlot::PlotLine("I", i_vals2.data(), i_vals2.size());
            ImPlot::PlotLine("Q", q_vals2.data(), q_vals2.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Magnitude FFT Symbol")) {
            ImPlot::SetupAxes("Subcarrier Index", "Magnitude");
            ImPlot::PlotLine("Abs(FFT)", sdr->test_bpsk_ofdm.fft_magnitude.data(), sdr->test_bpsk_ofdm.fft_magnitude.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Recovered Data Symbols")) {
            std::vector<double> i_vals3, q_vals3;
            i_vals3.reserve(sdr->test_bpsk_ofdm.rx_data_symbols.size());
            q_vals3.reserve(sdr->test_bpsk_ofdm.rx_data_symbols.size());
            for (size_t i = 0; i < sdr->test_bpsk_ofdm.rx_data_symbols.size(); i++) {
                i_vals3.push_back(sdr->test_bpsk_ofdm.rx_data_symbols[i].real());
                q_vals3.push_back(sdr->test_bpsk_ofdm.rx_data_symbols[i].imag());
            }
            ImPlot::PlotScatter("I/Q", i_vals3.data(), q_vals3.data(), i_vals3.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
}

void test_bpsk_ofdm_demod(sdr_global_t *sdr)
{
    ImGui::Text("TX bits: ");
    for (size_t i = 0; i < sdr->test_bpsk_ofdm.bit_array.size(); i++) {
        ImGui::Text("%d", sdr->test_bpsk_ofdm.bit_array[i]);
        ImGui::SameLine();
    }

    ImGui::NewLine();
    ImGui::NewLine();

    ImGui::Text("RX bits: ");
    for (size_t i = 0; i < sdr->test_bpsk_ofdm.demod_bit_array.size(); i++) {
        ImGui::Text("%d", sdr->test_bpsk_ofdm.demod_bit_array[i]);
        ImGui::SameLine();
    }

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::Text("Info:");
    ImGui::Text("Null: 12(6-6)");
    ImGui::Text("FFT size: %d", sdr->test_bpsk_ofdm.params.fft_size);
    ImGui::Text("CP len: %d", sdr->test_bpsk_ofdm.params.cp_len);
    ImGui::Text("TX samples: %zu", sdr->test_bpsk_ofdm.ofdm_tx_samples.size());
    ImGui::Text("Data carriers: %zu", sdr->test_bpsk_ofdm.params.data_carriers.size());
    ImGui::Text("Pilot carriers: %zu", sdr->test_bpsk_ofdm.params.pilot_carriers.size());

    int success_counter = 0;
    int compare_size = std::min(sdr->test_bpsk_ofdm.bit_array.size(), sdr->test_bpsk_ofdm.demod_bit_array.size());
    for (int i = 0; i < compare_size; i++)
    {
        if (sdr->test_bpsk_ofdm.bit_array[i] == sdr->test_bpsk_ofdm.demod_bit_array[i]) {
            success_counter++;
        }
    }

    if (compare_size > 0) {
        ImGui::Text("Success rate: %.2f%%", success_counter * 100.0f / compare_size);
    }
}

void show_realtime_ofdm_window(sdr_global_t *sdr)
{
    static int rows = 7;
    static int cols = 1;
    static float rratios[] = {5, 5, 5, 5, 5, 5, 5};
    static float cratios[] = {5, 5, 5, 5, 5, 5, 5};

    ImVec2 win_size = ImGui::GetWindowSize();
    win_size.y -= 50;
    win_size.x -= 50;

    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;

    if (ImPlot::BeginSubplots("Real Time OFDM", rows, cols, win_size, flags, rratios, cratios)) {

        if (ImPlot::BeginPlot("OFDM RX Raw Samples")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);

            std::vector<double> i_vals, q_vals;
            size_t plot_size = std::min(sdr->test_bpsk_ofdm.ofdm_rx_samples.size(), (size_t)1000);
            i_vals.reserve(plot_size);
            q_vals.reserve(plot_size);

            for (size_t i = 0; i < plot_size; i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.ofdm_rx_samples[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.ofdm_rx_samples[i].imag());
            }

            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("CFO Corrected OFDM Samples")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);

            std::vector<double> i_vals, q_vals;
            size_t plot_size = std::min(sdr->test_bpsk_ofdm.cfo_corrected_samples.size(), (size_t)1000);
            i_vals.reserve(plot_size);
            q_vals.reserve(plot_size);

            for (size_t i = 0; i < plot_size; i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.cfo_corrected_samples[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.cfo_corrected_samples[i].imag());
            }

            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Detected OFDM Symbol Without CP")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);

            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.size());
            q_vals.reserve(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.size());

            for (size_t i = 0; i < sdr->test_bpsk_ofdm.ofdm_symbol_no_cp.size(); i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.ofdm_symbol_no_cp[i].imag());
            }

            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Detected FFT Magnitude")) {
            ImPlot::SetupAxes("Subcarrier Index", "Magnitude");
            ImPlot::PlotLine(
                "Abs(FFT)",
                sdr->test_bpsk_ofdm.fft_magnitude.data(),
                sdr->test_bpsk_ofdm.fft_magnitude.size()
            );
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Pilot Phase Corrected FFT Symbol")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);

            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_bpsk_ofdm.phase_corrected_symbol.size());
            q_vals.reserve(sdr->test_bpsk_ofdm.phase_corrected_symbol.size());

            for (size_t i = 0; i < sdr->test_bpsk_ofdm.phase_corrected_symbol.size(); i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.phase_corrected_symbol[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.phase_corrected_symbol[i].imag());
            }

            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Recovered OFDM Data Symbols (Constellation)")) {
            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_bpsk_ofdm.equalized_data_symbols.size());
            q_vals.reserve(sdr->test_bpsk_ofdm.equalized_data_symbols.size());

            for (size_t i = 0; i < sdr->test_bpsk_ofdm.equalized_data_symbols.size(); i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.equalized_data_symbols[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.equalized_data_symbols[i].imag());
            }

            ImPlot::PlotScatter("I/Q", i_vals.data(), q_vals.data(), i_vals.size());
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("OFDM Phase-Corrected Data Symbols (Time Order)")) {
            ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Reverse);

            std::vector<double> i_vals, q_vals;
            i_vals.reserve(sdr->test_bpsk_ofdm.equalized_data_symbols.size());
            q_vals.reserve(sdr->test_bpsk_ofdm.equalized_data_symbols.size());

            for (size_t i = 0; i < sdr->test_bpsk_ofdm.equalized_data_symbols.size(); i++) {
                i_vals.push_back(sdr->test_bpsk_ofdm.equalized_data_symbols[i].real());
                q_vals.push_back(sdr->test_bpsk_ofdm.equalized_data_symbols[i].imag());
            }

            ImPlot::PlotLine("I", i_vals.data(), i_vals.size());
            ImPlot::PlotLine("Q", q_vals.data(), q_vals.size());
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }

    ImGui::Separator();
    ImGui::Text("Detected symbol start: %d", sdr->test_bpsk_ofdm.detected_symbol_start);
    ImGui::Text("Symbols in buffer: %d", sdr->test_bpsk_ofdm.detected_symbols_in_buffer);
    ImGui::Text("CP correlation metric: %.6f", sdr->test_bpsk_ofdm.detected_cp_metric);
    ImGui::Text("Estimated CFO: %.9f", sdr->test_bpsk_ofdm.estimated_cfo_rad_per_sample);
    ImGui::Text("Estimated common phase: %.9f", sdr->test_bpsk_ofdm.estimated_common_phase);

    int success_counter = 0;
    int compare_size = std::min(
        sdr->test_bpsk_ofdm.bit_array.size(),
        sdr->test_bpsk_ofdm.demod_bit_array.size()
    );

    for (int i = 0; i < compare_size; i++) {
        if (sdr->test_bpsk_ofdm.bit_array[i] == sdr->test_bpsk_ofdm.demod_bit_array[i]) {
            success_counter++;
        }
    }

    if (compare_size > 0) {
        ImGui::Text("Realtime OFDM success rate: %.2f%%", success_counter * 100.0f / compare_size);
    }
}