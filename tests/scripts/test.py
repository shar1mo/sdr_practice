import numpy as np
import matplotlib.pyplot as plt
from numpy import arange, sqrt, cos, sin, pi, zeros
from scipy import signal
# Открываем файл для чтения
name = "/home/nsk/dev/test/sdrLessons/build/txdata_bpsk_barker13.pcm"
def srrc(syms, beta, P, t_off=0):

    # s = (4*beta/np.sqrt(P)) scales SRRC
    # syms  = Half of Total Number of Symbols
    P = P
    beta = beta
    length_SRRC = P * syms * 2
    start = float((-length_SRRC / 2) + 1e-8 + t_off)
    stop = float((length_SRRC / 2) + 1e-8 + t_off)
    step = float(1)
    k = arange(start=start, stop=stop + 1, step=step, dtype=float)
    if beta == 0:
        beta = 1e-8
    denom = (pi * (1 - 16 * ((float(beta) * k / float(P))**2)))
    # s = (np.cos((1 + beta) * np.pi * k / P) + (np.sin(
    #     (1 - beta) * np.pi * k / P) / (4 * beta * k / P))) / denom
    s = (4 * beta / sqrt(P)) * (cos((1 + beta) * pi * k / P) + (sin(
        (1 - beta) * pi * k / P) / (4 * beta * k / P))) / denom
    return s

# def srrc(sps, span, roll_off):
#     """
#     Designs a Square Root Raised Cosine (SRRC) FIR filter.

#     Parameters:
#     sps (int): Samples per symbol.
#     span (int): Filter span in symbols (affects filter length: span*sps + 1).
#     roll_off (float): Roll-off factor (alpha).
#     """
#     # Total number of taps
#     num_taps = sps * span *2
#     # Time vector for the impulse response
#     t = np.arange(num_taps) - (num_taps - 1) / 2
#     t /= sps

#     h = np.zeros(num_taps)
    
#     # Handle the main cases and edge conditions (division by zero)
#     for i in range(num_taps):
#         if t[i] == 0:
#             h[i] = 1.0 + roll_off * (4/np.pi - 1) # Value at the center tap (t=0)
#         elif abs(abs(4 * roll_off * t[i]) - 1) < 1e-6:
#             # Value at t = +/- Ts/(4*alpha)
#             h[i] = roll_off / np.sqrt(2) * ((1 + 2/np.pi) * np.sin(np.pi/(4*roll_off)) + (1 - 2/np.pi) * np.cos(np.pi/(4*roll_off)))
#         else:
#             # Main formula
#             h[i] = (np.sin(np.pi * t[i] / (1/sps) * (1 - roll_off)) + 4 * roll_off * t[i] / (1/sps) * np.cos(np.pi * t[i] / (1/sps) * (1 + roll_off))) / \
#                    (np.pi * t[i] / (1/sps) * (1 - (4 * roll_off * t[i] / (1/sps))**2))
    
#     # Normalize to unity energy or unit gain if required.
#     # For a transmit filter, often gain at f=0 is set to 1.
#     h = h / np.sum(h) 
    
#     return h

def correlate(a, v):
    n = len(a)
    m = len(v)
    result = []
    for i in range(n - m + 1):
        s = 0
        for j in range(m):
            s += a[i + j] * v[j].conjugate()
        result.append(s)
    return result

def quantalph_distance(symbol_array, alphabet):
    # symbolArray = downsampled symbols
    # alphabet = coded alphabet, it should be pass with symbols values which are greater than zero
    # distance = the distance on I and Q axis between the max and min QAM4_2 symbols
    # threshold = this is equal to the 1/4 of the total distance. This distance in the IQ axis is considered as a circle whose diameter is equal to the distance
    ## the radius of that circle is equal to distance/4. this radius is divided into 6 equal points and the distance of the third point to the origin is set as threshold
    min_alphabet = min(alphabet)
    max_alphabet = max(alphabet)
    min_point = min(symbol_array)
    max_point = max(symbol_array)
    distance = abs(min_point) + abs(max_point)
    threshold = 1 * distance / 4
    quantized_symbols = zeros(len(symbol_array), dtype=complex)
    threshold_array = threshold * np.exp(
        1j * 2 * pi * arange(0, 1, 1 / len(symbol_array)))
    for i in range(len(symbol_array)):
        if abs(symbol_array[i]) < threshold:
            quantized_symbols[i] = np.sign(np.real(
                symbol_array[i])) * min_alphabet + 1j * np.sign(
                    np.imag(symbol_array[i])) * min_alphabet
        else:
            quantized_symbols[i] = np.sign(np.real(
                symbol_array[i])) * max_alphabet + 1j * np.sign(
                    np.imag(symbol_array[i])) * max_alphabet
    return quantized_symbols

def ted(matched, samples_per_symbol: int):
    BnTs = 0.01
    Kp = 0.002
    zeta = np.sqrt(2) / 2
    theta = (BnTs / samples_per_symbol) / (zeta + (0.25 / zeta))
    K1 = -4 * zeta * theta / ((1 + 2 * zeta * theta + theta**2) * Kp)
    K2 = -4 * theta**2 / ((1 + 2 * zeta * theta + theta**2) * Kp)
    tau = 0
    p2 = 0
    errof = []
    symb_samples = []

    for i in range(0, len(matched) - samples_per_symbol, samples_per_symbol):
        err = ((matched[i + samples_per_symbol + tau].real - matched[i + tau].real) * matched[i + (samples_per_symbol // 2) + tau].real +
               (matched[i + samples_per_symbol + tau].imag - matched[i + tau].imag) * matched[i + (samples_per_symbol // 2) + tau].imag)
        p1 = err * K1
        p2 = p2 + p1 + err * K2

        if p2 > 1:
            p2 -= 1
        if p2 < -1:
            p2 += 1

        tau = int(round(p2 * samples_per_symbol))
        errof.append(i + samples_per_symbol + tau)
    print(errof)
    for idx in (errof):
        symb_samples.append(matched[idx])

    return symb_samples

data = []
imag = []
real = []
count = []
counter = 0
rx_int = []
r = 0
im = 0
with open(name, "rb") as f:
    index = 0
    while (byte := f.read(2)):
        if(index %2 == 0):
            real.append(int.from_bytes(byte, byteorder='little', signed=True))
            counter += 1
            count.append(counter)
        else:
            imag.append(int.from_bytes(byte, byteorder='little', signed=True))
        index += 1

np_real = np.array(real)
np_imag = np.array(imag)
rx_int = np_real + np_imag * 1j
# System Parameters
tx_LO_frequency = int(700e6)
rx_LO_frequency = int(750e6)
tx_rx_difference = int(tx_LO_frequency - rx_LO_frequency)
band_pass_filter_bandwidth = int(0.8e6)
bandwidth = int(2e6)
sampling_rate = int(4e6)
number_of_samples = 2**16
rx_int = rx_int[149000: 153000]
# rx_int = rx_int[2830:4380]
# np_arr = np.array(rx_int)
print(len(rx_int))
Ts = 1 / sampling_rate

#result_tile = np.tile(np_arr, 5)
rx = rx_int
print(len(rx))
rx = rx / 2**11

#SRRC Generation
P = 16
sps = 16
nOfSL = 5
fs = 4e6
beta = 0.75
# Create our raised-cosine filter
num_taps = nOfSL * P * 2
h = srrc(nOfSL, P, beta)

# Filter our signal, in order to apply the pulse shaping
samples = np.convolve(rx, h)

plt.figure(2)
plt.plot(np.real(samples), '.-')
plt.plot(np.imag(samples), '.-')
plt.grid(True)

plt.figure(3)
plt.plot(np.real(samples), np.imag(samples), '.')

arr_copy = np.array(samples.copy())
samples_before_fft = arr_copy**2

psd = np.fft.fftshift(np.abs(np.fft.fft(samples_before_fft)))
f = np.arange(-fs / 2.0, fs / 2.0, fs / len(psd))
max_freq = f[np.argmax(psd)]
print("max freq = ", max_freq)

Ts = 1/fs # calc sample period
t = np.arange(0, Ts*len(samples), Ts) # create time vector
print(len(samples))
samples = samples * np.exp(-1j*2*np.pi*max_freq*t/2.0)

plt.figure(8)
plt.plot(np.real(samples), '.-')
plt.plot(np.imag(samples), '.-')
plt.grid(True)
plt.figure()
plt.plot(np.real(samples), np.imag(samples), '.')

n_poly = 32
# Symbol sync, using what we did in sync chapter
samples_interpolated = signal.resample_poly(samples, n_poly, 1) # we'll use 32 as the interpolation factor, arbitrarily chosen, seems to work better than 16
sps = 16

mu = 0.01 
out = np.zeros(len(samples) + 10, dtype=np.complex64)
out_rail = np.zeros(len(samples) + 10, dtype=np.complex64)
i_in = 0 
i_out = 2 
while i_out < len(samples) and i_in+16 < len(samples):
    out[i_out] = samples[i_in]
 
    out_rail[i_out] = int(np.real(out[i_out]) > 0) + 1j*int(np.imag(out[i_out]) > 0)
    x = (out_rail[i_out] - out_rail[i_out-2]) * np.conj(out[i_out-1])
    y = (out[i_out] - out[i_out-2]) * np.conj(out_rail[i_out-1])
    mm_val = np.real(y - x)
    mu += sps + 0.01*mm_val
    i_in += int(np.floor(mu))
    mu = mu - np.floor(mu) 
    i_out += 1 
samples = out[2:i_out] 


plt.figure()
plt.plot(np.real(samples), '.-')
plt.plot(np.imag(samples), '.-')
plt.grid(True)

plt.figure()
plt.plot(np.real(samples), np.imag(samples), '.')




N = len(samples)
phase = 0
freq = 0
# These next two params is what to adjust, to make the feedback loop faster or slower (which impacts stability)
alpha = 8.0
beta = 0.02
out = np.zeros(N, dtype=np.complex64)
freq_log = []
for i in range(N):
    out[i] = samples[i] * np.exp(-1j*phase) # adjust the input sample by the inverse of the estimated phase offset
    error = np.real(out[i]) * np.imag(out[i]) # This is the error formula for 2nd order Costas Loop (e.g. for BPSK)

    # Advance the loop (recalc phase and freq offset)
    freq += (beta * error)
    freq_log.append(freq * sampling_rate / (2*np.pi)) # convert from angular velocity to Hz for logging
    phase += freq + (alpha * error)

    # Optional: Adjust phase so its always between 0 and 2pi, recall that phase wraps around every 2pi
    while phase >= 2*np.pi:
        phase -= 2*np.pi
    while phase < 0:
        phase += 2*np.pi
x = out

# Plot freq over time to see how long it takes to hit the right offset
plt.figure()
plt.plot(freq_log,'.-')
    
plt.figure()
plt.plot(np.real(out), '.-')
plt.plot(np.imag(out), '.-')
plt.grid(True)

plt.figure()
plt.plot(np.real(out), np.imag(out), '.')

quantized_symbols =  out / np.max(out)  #quantalph_distance(out, np.array([-1.1, 1.1]))
plt.figure()
plt.plot(np.real(quantized_symbols), '.-')
plt.plot(np.imag(quantized_symbols), '.-')
plt.grid(True)

header = np.array([])
barker_real = [1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1]
barker_imag = [1j, 1j, 1j, 1j, 1j, -1j, -1j, 1j, 1j, -1j, 1j, -1j, 1j]
header = np.add(barker_real, barker_imag)
trigger = 4
real_symbols = np.real(quantized_symbols)
imag_symbols = np.imag(quantized_symbols)
header_real = np.real(header)
header_imag = np.imag(header)
correlation_real = np.correlate(real_symbols, header_real, 'full') / len(real_symbols) **2
correlation_imag = np.correlate(imag_symbols, header_imag, 'full')
imag_correlation_indices = zeros(80)
imag_correlation_values = zeros(80)
real_correlation_indices = zeros(80)
real_correlation_values = zeros(80)
i = 0
k = 0
for index, value in np.ndenumerate(correlation_real):
    if abs(value) >= trigger:
        real_correlation_indices[i] = index[0]
        real_correlation_values[i] = value
        i += 1
for index, value in np.ndenumerate(correlation_imag):
    if abs(value) >= trigger:
        imag_correlation_indices[k] = index[0]
        imag_correlation_values[k] = value
        k += 1

plt.figure()
plt.stem(correlation_real, 'r')
plt.stem(correlation_imag, 'b')
plt.grid(True)
print(imag_correlation_values, real_correlation_values)
print(imag_correlation_indices, real_correlation_indices)

a = 59
b = 184
plt.figure()
plt.plot(np.real(quantized_symbols[a:a+b]), '.-')
plt.plot(np.imag(quantized_symbols[a:a+b]), '.-')
plt.grid(True)

plt.figure()
plt.plot(np.real(quantized_symbols[a:a+b]), np.imag(quantized_symbols[a:a+b]), '.')

bits = (np.real(quantized_symbols[a:a+b]) > 0).astype(int) # 1's and 0's
# Differential decoding, so that it doesn't matter whether our BPSK was 180 degrees rotated without us realizing it
# bits = (bits[1:] - bits[0:-1]) % 2
# bits = bits.astype(np.uint8) # for decoder

print(bits, len(bits))
from bitarray import bitarray

# Example bit array (must be a multiple of 8 bits in length)
# This example represents the ASCII characters 'H', 'i'
# 'H' is 72 (01001000 in binary), 'i' is 105 (01101001 in binary)
result_mixed = ''.join(str(item) for item in bits)
bits = bitarray(result_mixed)

# Convert the bitarray to a bytes object
bytes_data = bits.tobytes()

# Decode the bytes object to a string
char_string = bytes_data.decode('ascii') # or 'utf-8', depending on your data

print(f"Bytes: {bytes_data}")
print(f"Characters: {char_string}")

plt.show()

