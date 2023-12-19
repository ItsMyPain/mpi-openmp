import numpy as np
from matplotlib import pyplot as plt

data = np.array([
    [1, 136.810997],
    [2, 68.401025],
    [3, 45.618507],
    [4, 34.451344],
    [6, 22.961625],
    [8, 18.660289],
    [9, 18.091438],
    [10, 14.437609],
    [12, 13.509964],
    [14, 11.151752],
    [15, 10.731696],
    [16, 10.265020],
    [18, 7.724943],
    [20, 8.175292],
    [21, 7.781359],
    [24, 6.976555],
    [28, 5.960468],
])

y_pract = []
x_pract = [i[0] for i in data]
for i in data:
    y_pract.append(data[0][1] / i[1])

x = np.arange(1, 29)
y_teor = x

plt.plot(x, y_teor, label='Теор.')
plt.plot(x_pract, y_pract, label='Практ.')
plt.legend()
plt.grid()
plt.title('Зависимость ускорения от числа исполнителей')
plt.ylabel('Ускорение, раз')
plt.xlabel('Количество исполнителей, шт.')
plt.show()
