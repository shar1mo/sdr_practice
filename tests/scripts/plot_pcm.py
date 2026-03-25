import numpy as np
import matplotlib.pyplot as plt

# Открываем файл для чтения
name = "/home/nsk/dev/test/sdrLessons/build/txdata.pcm"

data = []
imag = []
real = []
counter = 0
count = []
rx_int = []
r = 0
im = 0
with open(name, "rb") as f:
    index = 0
    while (byte := f.read(2)):
        if(index %2 == 0):
            real.append(int.from_bytes(byte, byteorder='little', signed=True))
            r = int.from_bytes(byte, byteorder='little', signed=True)
            counter += 1
            count.append(counter)
        else:
            imag.append(int.from_bytes(byte, byteorder='little', signed=True))
            im = int.from_bytes(byte, byteorder='little', signed=True)
        index += 1
        
# Инициализируем список для хранения данных
# for i in range(len(real)):
#     print(real[i], imag[i], end=' ')

# fig, axs = plt.subplots(2, 1, layout='constrained')


# plt.figure(1)
# # axs[1].plot(count, np.abs(data),  color='grey')  # Используем scatter для диаграммы созвездия
# plt.plot(count,(imag),color='red')  # Используем scatter для диаграммы созвездия
# plt.plot(count,(real), color='blue')  # Используем scatter для диаграммы созвездия
# plt.show()


count = 0
file = open("out.txt", "w")
file.write("static std::vector<std::complex<double>> test_rx_samples_bpsk_barker13 = {")
for i in range(149000, 243000):
    file.write("{")
    file.write(str(real[i]))
    file.write(", ")
    file.write(str(imag[i]))
    file.write("},")
    count += 1
    if(count % 20 == 0):
        file.write("\n")
file.write("\n")
file.write("};")
file.close()
print(count)
