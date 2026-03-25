#include <iostream>
#include <vector>
#include <algorithm> 
#include <complex.h>
#include <numbers> 

// std::vector<double> convolve(std::vector<double> &a, std::vector<double> &b){
    
//     std::vector<double> result;
//     for(int n = 0; n < a.size(); n += 10)
//     {
//         double summ = 0;
//         for(int m = 0; m < b.size(); m++)
//         {
//             summ += a[m] * b[n - m];
//         }
//         result.push_back(summ);
//     }
//     return result;
// }

std::vector<double> correlate_manual(const std::vector<double>& a, const std::vector<double>& v, const std::string& mode = "same") {
    int n1 = a.size();
    int n2 = v.size();

    // Reverse the second array for cross-correlation
    std::vector<double> v_rev = v;
    std::reverse(v_rev.begin(), v_rev.end());

    int output_size;
    int start_index;

    if (mode == "full") {
        output_size = n1 + n2 - 1;
        start_index = -(n2 - 1);
    } else if (mode == "same") {
        output_size = n1;
        // For "same" mode, the output is centered with the same size as 'a'.
        // The start index corresponds to the lag where 'a' and 'v' overlap appropriately.
        // This is a bit complex to calculate precisely based on the definition of "same" mode in numpy, 
        // but the result should match the size of 'a'.
        // A common implementation for "same" involves taking the middle part of the "full" result.
    } else { // Default is "valid"
        output_size = std::max(0, n1 - n2 + 1);
        start_index = n2 - 1;
    }
    
    // For manual implementation, it's easier to compute "full" and then trim.
    std::vector<double> full_result(n1 + n2 - 1, 0.0);
    for (int i = 0; i < n1; ++i) {
        for (int j = 0; j < n2; ++j) {
            full_result[i + j] += a[i] * v_rev[j]; // v_rev[j] is effectively v[n2 - 1 - j]
        }
    }

    // Trim the "full" result based on the mode
    std::vector<double> result;
    if (mode == "full") {
        result = full_result;
    } else if (mode == "same") {
        int start = (n2 - 1) / 2;
        result.reserve(n1);
        for (int i = 0; i < n1; ++i) {
            result.push_back(full_result[i + start]);
        }
    } else { // "valid"
        int start = n2 - 1;
        result.reserve(output_size);
        for (int i = 0; i < output_size; ++i) {
            result.push_back(full_result[i + start]);
        }
    }

    return result;
}

std::vector<double> correlate_valid(const std::vector<int>& a, const std::vector<int>& v) {
    if (a.size() < v.size()) {
        // NumPy's correlate(a, v) computes correlation of a with v
        // The result length for 'valid' mode is max(M, N) - min(M, N) + 1
        // It's often clearer to ensure 'a' is the longer sequence
        // For simplicity here, we assume a is long enough.
        // A more robust implementation would handle this case or swap a and v
    }

    int M = a.size();
    int N = v.size();
    int K = M - N + 1; // Size of the result for 'valid' mode
    std::vector<double> result(K, 0.0);

    // NumPy correlates a with v (convolution of a with time-reversed v)
    // The formula is c_k = sum_n a[n + k] * conj(v[n])
    // For real numbers, conj is not needed.
    for (int k = 0; k < K; ++k) {
        double sum = 0.0;
        for (int n = 0; n < N; ++n) {
            sum += a[n + k] * v[n];
        }
        result[k] = sum;
    }

    return result;
}

std::vector< std::complex<double> > convolve(std::vector<std::complex<double>> &upsampled, std::vector<double> &b) {
    std::vector <std::complex<double>> convolved;
    for (int n = 0; n < upsampled.size(); ++n) 
        {
            std::complex<double> sum(0.0f, 0.0f);
            for (int k = 0; k < b.size(); ++k) 
            {
                int idx = n - k;
                if (idx >= 0 && idx < upsampled.size()) { 
                sum += upsampled[idx] * b[k];
            }
        }
        convolved.push_back(sum);
        }
    return convolved;
}

std::vector<double> arange(double start, double stop, double step) {
    std::vector<double> result;
    for (double val = start; val < stop; val += step) {
        result.push_back(val);
    }
    return result;
}

void fftshift_1d(std::vector<double> &data, int N) {
    // Calculate the midpoint for the shift
    int shift_len = (N + 1) / 2; // Correctly handles both even and odd N

    // Allocate a temporary array to hold the shifted data
    std::vector<double> temp;
    temp.resize(N);

    // Perform the circular shift
    for (int i = 0; i < N; i++) {
        // The destination index using modulo arithmetic for circularity
        int dst = (i + shift_len) % N;
        temp[dst] = data[i]; // Real part
    }

    // Copy the shifted data back to the original array
    for (int i = 0; i < N; i++) {
        data[i] = temp[i];
    }
}

std::vector<std::complex<double>> correlate(const std::vector<std::complex<double>>& a, const std::vector<std::complex<double>>& v) {
    int n = a.size();
    int m = v.size();
    std::vector<std::complex<double>> result;
    result.reserve(n - m + 1);
    for (int i = 0; i <= n - m; ++i) {
        std::complex<double> s = 0;
        for (int j = 0; j < m; ++j) {
            s += a[i + j] * std::conj(v[j]);
        }
        result.push_back(s);
    }
    return result;
}

// Function to perform a 1D fftshift in-place on a complex array
// void fftshift_1d(fftw_complex* data, int N) {
//     // Calculate the midpoint for the shift
//     int shift_len = (N + 1) / 2; // Correctly handles both even and odd N

//     // Allocate a temporary array to hold the shifted data
//     fftw_complex* temp = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
//     if (temp == NULL) {
//         fprintf(stderr, "Memory allocation failed for fftshift temp array\n");
//         exit(EXIT_FAILURE);
//     }

//     // Perform the circular shift
//     for (int i = 0; i < N; i++) {
//         // The destination index using modulo arithmetic for circularity
//         int dst = (i + shift_len) % N;
//         temp[dst][0] = data[i][0]; // Real part
//         temp[dst][1] = data[i][1]; // Imaginary part
//     }

//     // Copy the shifted data back to the original array
//     for (int i = 0; i < N; i++) {
//         data[i][0] = temp[i][0];
//         data[i][1] = temp[i][1];
//     }

//     // Free the temporary array
//     fftw_free(temp);
// }


// std::vector<std::complex<double>> convolve(std::vector<std::complex<double>> &a, std::vector<double> &b)
// {
//     std::vector<std::complex<double>> result;
//     result.resize(a.size());
//     std::complex<double> val;
//     if (a.size() > b.size())
//     {
//         int N = b.size();
//         for (int i = 0; i < a.size(); i += N){
//             for (int j = 0; j < N; j++){
//                 double summ_real = 0.0;
//                 double summ_imag = 0.0;
//                 for (int m = 0; m < b.size(); m++)
//                 {
//                     // TODO: multiply not working
//                     double real = a[i + m].real() * b[j - m];
//                     double imag = a[i+m].imag() * b[j - m];
//                     summ_real += real;
//                     summ_imag += imag;
//                 }
//                 val = (summ_real, summ_imag);
//                 result[i + j] = val;
//             }
//         }
//     }
//     return result;
// }

/*
Test set from Python NumPy
import numpy as np
import matplotlib.pyplot as plt
x = np.linspace(-4, 4, 41)
np.sinc(x)
array([-3.89804309e-17,  -4.92362781e-02,  -8.40918587e-02, # may vary
        -8.90384387e-02,  -5.84680802e-02,   3.89804309e-17,
        6.68206631e-02,   1.16434881e-01,   1.26137788e-01,
        8.50444803e-02,  -3.89804309e-17,  -1.03943254e-01,
        -1.89206682e-01,  -2.16236208e-01,  -1.55914881e-01,
        3.89804309e-17,   2.33872321e-01,   5.04551152e-01,
        7.56826729e-01,   9.35489284e-01,   1.00000000e+00,
        9.35489284e-01,   7.56826729e-01,   5.04551152e-01,
        2.33872321e-01,   3.89804309e-17,  -1.55914881e-01,
       -2.16236208e-01,  -1.89206682e-01,  -1.03943254e-01,
       -3.89804309e-17,   8.50444803e-02,   1.26137788e-01,
        1.16434881e-01,   6.68206631e-02,   3.89804309e-17,
        -5.84680802e-02,  -8.90384387e-02,  -8.40918587e-02,
        -4.92362781e-02,  -3.89804309e-17])

This func result:
        {-3.89817e-17 -0.0492363 -0.0840919 
        -0.0890384 -0.0584681 3.89817e-17 
        0.0668207 0.116435 0.126138 
        0.0850445 -3.89817e-17 -0.103943 
        -0.189207 -0.216236 -0.155915 3.89817e-17 
        0.233872 0.504551 0.756827 0.935489 1 0.935489 
        0.756827 0.504551 0.233872 3.89817e-17 
        -0.155915 -0.216236 -0.189207 -0.103943 
        -3.89817e-17 0.0850445 0.126138 0.116435 
        0.0668207 3.89817e-17 -0.0584681 -0.0890384 
        -0.0840919 -0.0492363 -3.89817e-17}
*/
std::vector<double> sinc(const std::vector<double>& x) {
    std::vector<double> y(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
        double val = (x[i] == 0) ? 1.0e-20 : x[i];
        y[i] = M_PI * val;
    }
    std::vector<double> result(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
        result[i] = std::sin(y[i]) / y[i];
    }
    return result;
}

int f()
{
    static int i;
    return ++i;
}

std::vector<double> linspace(double start, double end, int num) {
    std::vector<double> linspaced;
    if (num == 0) { return linspaced; }
    if (num == 1) {
        linspaced.push_back(start);
        return linspaced;
    }
    linspaced.resize(num);
    double delta = (end - start) / (num - 1);
    for (int i = 0; i < num; ++i) {
        double c = delta * static_cast<double>(i);
        linspaced[i] = start + c;
    }
    return linspaced;
}