#include "bit_generation.h"
#include "pulse_shaping.h"
#include "matplotlibcpp.h"
#include "modulation.h"
#include "math_operations.h"
#include <complex.h>

namespace plt = matplotlibcpp;


int main(){

    // 1. Генерация случайного набора бит
    std::cout << "1. Генерация случайного набора бит" << std::endl;
    std::vector<int> bit_array = generate_bit_array(10);
    plot_vector(bit_array, "Генерируем случайный битовый набор", "Индекс массива", "Значение Бита", false);

    // 2. Модуляция\манипуляция BPSK\QPSK\N_QAM и др
    std::cout << "1. Генерация случайного набора бит" << std::endl;
    std::vector<std::complex<double>> modulated_array = modulate(bit_array, 1);
    plot_vector(modulated_array, "Модулированные I\Q сэмплы (BPSK)", "Индекс массива", "Значение сэмпла", false);

    // 3. Формирование Символов (upsampling)
    std::cout << "1. Генерация случайного набора бит" << std::endl;
    std::vector<std::complex<double>> upsampled_bit_array = upsample(modulated_array, 10);
    plot_vector(upsampled_bit_array, "I\Q сэмплы  после Upsampling", "Индекс массива", "Значение сэмпла", false);

    // 4. Pulse shaping (формирующий фильтрб matched filter)
    std::cout << "1. Генерация случайного набора бит" << std::endl;
    std::vector<double> filter = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    std::vector<std::complex<double>> match_filtered = convolve(upsampled_bit_array, filter);
    plot_vector(match_filtered, "I\Q сэмплы  после Pulse Shaping", "Индекс массива", "Значение сэмпла", false);

    // Это для корректного отображения всех графиков одновременно. 
    std::vector<int> exit_array = {0, 0, 0};
    plt::plot(exit_array);
    plt::show();
    return 0;
}