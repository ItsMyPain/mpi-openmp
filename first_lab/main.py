import numpy as np
from matplotlib import pyplot as plt

BYTES_IN_INT = 4

times_one_node = np.array([319.440000, 335.220000, 326.950000, 317.610000, 318.910000, 354.930000])
n_one_node = 2e8
times_two_node = np.array([92.270000, 93.760000, 94.030000, 93.960000, 83.440000, 82.840000, 84.110000])
n_two_node = 2e6

t_1e4x1e6_one_node = np.array([48.610000, 49.330000, 75.040000, 68.620000, 56.740000])
t_1e4x1e6_two_node = np.array([66.840000, 71.990000, 68.290000, 66.070000, 68.340000])
n_1e4x1e6 = 2e4
x_1e6 = 1e6 * BYTES_IN_INT
t_100x1e8_one_node = np.array([58.270000, 63.400000, 72.450000, 65.620000, 43.380000, 65.040000, 65.220000, 44.120000])
t_100x1e8_two_node = np.array([69.040000, 76.800000, 67.690000, 73.840000, 67.600000])
n_100x1e8 = 200
x_1e8 = 1e8 * BYTES_IN_INT

t_pract_one_node = np.array(
    [0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.030000, 0.030000, 0.050000, 0.060000, 0.100000,
     0.150000, 0.250000, 0.410000, 0.610000, 1.040000, 1.910000, 3.650000, 7.150000, 15.460000, 32.920000])
n_pract_one_node = 2e4
y_pract_one_node = t_pract_one_node / n_pract_one_node
x_pract = np.array([2 ** i for i in range(0, 21)]) * BYTES_IN_INT

t_pract_two_nodes = np.array(
    [0.540000, 0.520000, 0.520000, 0.520000, 0.510000, 0.520000, 0.520000, 0.540000, 0.540000, 0.790000, 0.900000,
     0.960000, 1.200000, 1.600000, 2.280000, 3.610000, 5.960000, 10.040000, 17.750000, 33.610000, 68.790000])
n_pract_two_nodes = 2e4
y_pract_two_nodes = t_pract_two_nodes / n_pract_one_node

latency_time_one_node = times_one_node.mean() / n_one_node
latency_time_two_node = times_two_node.mean() / n_two_node
y_1e6_one_node = t_1e4x1e6_one_node.mean() / n_1e4x1e6
y_1e8_one_node = t_100x1e8_one_node.mean() / n_100x1e8
k_one_node = (y_1e8_one_node - y_1e6_one_node) / (x_1e8 - x_1e6)
y_1e6_two_node = t_1e4x1e6_two_node.mean() / n_1e4x1e6
y_1e8_two_node = t_100x1e8_two_node.mean() / n_100x1e8
k_two_node = (y_1e8_two_node - y_1e6_two_node) / (x_1e8 - x_1e6)
print(1 / k_one_node, 1 / k_two_node)
print(latency_time_one_node, latency_time_two_node)
h_x = 1
x_end = 1e6
x_teor = np.arange(0, x_end + h_x / 2, h_x)
y_teor_one_node = k_one_node * x_teor + latency_time_one_node
y_teor_two_nodes = k_two_node * x_teor + latency_time_two_node


def transfer_time():
    yscale = 1e6
    plt.plot(x_teor, y_teor_one_node * yscale, label='Внутри узла, теор.')
    plt.plot(x_teor, y_teor_two_nodes * yscale, label='Между узлами, теор')
    plt.plot(x_pract, y_pract_one_node * yscale, label='Внутри узла, практ.')
    plt.plot(x_pract, y_pract_two_nodes * yscale, label='Между узлами, практ.')
    plt.legend()
    plt.grid()
    plt.title('Время передачи данных')
    plt.ylabel('Время передачи, мкс')
    plt.xlabel('Размер данных, байт')
    plt.yscale('log')
    plt.xscale('log')
    plt.show()


def bandwight():
    yscale = 1e-6
    plt.plot(x_teor, x_teor / y_teor_one_node * yscale, label='Внутри узла, теор.')
    plt.plot(x_teor, x_teor / y_teor_two_nodes * yscale, label='Между узлами, теор')
    plt.plot(x_pract, x_pract / y_pract_one_node * yscale, label='Внутри узла, практ.')
    plt.plot(x_pract, x_pract / y_pract_two_nodes * yscale, label='Между узлами, практ.')
    plt.legend()
    plt.grid()
    plt.title('Пропускная способность')
    plt.ylabel('Пропускная способность, Мбайт/с')
    plt.xlabel('Размер данных, байт')
    plt.yscale('log')
    plt.xscale('log')
    plt.show()

transfer_time()
bandwight()
