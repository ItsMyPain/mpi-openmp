import numpy as np
import pandas as pd
from matplotlib import pyplot as plt


class KNN:
    x: np.ndarray
    y: np.array

    def __init__(self, k=5):
        self.k = k

    def _normalize(self, x: pd.DataFrame):
        self.mean = x.mean()
        self.std = x.std()
        return (x - self.mean) / self.std

    def fit(self, data: pd.DataFrame):
        self.x = self._normalize(data[['x1', 'x2']])
        self.y = data['y']

    def predict(self, x0):
        x0 = (x0 - self.mean) / self.std
        x = self.x - x0
        data = pd.DataFrame(data=dict(x=x['x1'] ** 2 + x['x2'] ** 2, y=self.y))
        data = data.sort_values(by='x')['y'][:self.k]
        return data.value_counts().index[0]


N = 100
class_1 = pd.DataFrame(data=dict(x1=-np.random.rand(N) + 4, x2=-np.random.rand(N) + 4, y=np.zeros(N)))
class_2 = pd.DataFrame(data=dict(x1=-np.random.rand(N) + 5, x2=-np.random.rand(N) + 5, y=np.ones(N)))

dataset = pd.concat([class_1, class_2])
X0 = np.array([3.2, 4.5])

model = KNN()
model.fit(dataset)
predict = model.predict(X0)
print(predict)

plt.scatter(class_1['x1'], class_1['x2'])
plt.scatter(class_2['x1'], class_2['x2'])
plt.scatter(X0[0], X0[1])
plt.show()
