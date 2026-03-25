#include <vector>
#include <complex.h>

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

template<typename T> T arange(T start, T stop, T step = 1) {
    std::vector<T> values;
    for (T value = start; value < stop; value += step) {
        values.push_back(value);
    }
    return values;
}

std::vector<std::complex<double>> convolve(std::vector<std::complex<double>> &a, std::vector<double> &b);
std::vector<int> correlate_valid(const std::vector<int> &a, const std::vector<int> &v);
std::vector<double> correlate_manual(const std::vector<double> &a, const std::vector<double> &v, const std::string &mode = "same");
std::vector<std::complex<double>> correlate(const std::vector<std::complex<double>> &a, const std::vector<std::complex<double>> &v);

std::vector<double> convolve(std::vector<double> &a, std::vector<double> &b);
std::vector<double> sinc(const std::vector<double> &x);
std::vector<double> linspace(double start, double end, int num);
std::vector<double> arange(double start, double stop, double step);

void fftshift_1d(std::vector<double> &data, int N);
