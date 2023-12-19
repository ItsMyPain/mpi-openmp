import numpy as np
import pandas as pd


class KNN:
    k: int
    x: np.ndarray
    y: np.array
    mean: float
    std: float
    classes: np.array
    num_classes: int

    def __init__(self, k=5):
        self.k = k

    def _normalize(self, data: np.ndarray) -> np.ndarray:
        return (data - self.mean) / self.std

    def fit(self, data: pd.DataFrame, target: pd.Series):
        if not isinstance(data, pd.DataFrame):
            raise ValueError('data must be a DataFrame')
        if not isinstance(target, pd.Series):
            raise ValueError('target must be a Series')

        x = data.values
        self.mean = x.mean()
        self.std = x.std()
        self.x = self._normalize(x)
        self.y = target.values
        self.classes = np.unique(self.y)
        self.num_classes = self.classes.shape[0]

    def predict(self, x: pd.DataFrame):
        if not isinstance(x, pd.DataFrame):
            raise ValueError('x must be a DataFrame')

        x = self._normalize(x.values)
        props = []
        for x0 in x:
            data = np.array([np.power(self.x - x0, 2).sum(axis=1), self.y]).T
            data = data[data[:, 0].argsort()].T[1][:self.k]
            labels, counts = np.unique(data, return_counts=True)
            indexes = np.where(np.in1d(self.classes, labels))[0]
            p = np.zeros(self.num_classes)
            p[indexes] = counts / self.k
            props.append(p)
        return np.array(props)


def get_data(filename: str, train_size=0.8, random_state=42):
    np.random.seed(random_state)
    dataset = pd.read_csv(filename, sep=',')
    dataset = dataset.replace({'variety': {'Setosa': 0, 'Versicolor': 1}})
    ln = dataset.shape[0]
    idx = set(range(ln))
    train_idx = set(np.random.choice(list(idx), int(train_size * ln), replace=False))
    test_idx = idx - train_idx
    test_data = dataset.iloc[list(test_idx)]
    train_data = dataset.iloc[list(train_idx)]
    return (train_data.drop('variety', axis=1), test_data.drop('variety', axis=1),
            train_data['variety'], test_data['variety'])


def get_metrics(y_pred: np.ndarray, y_true: np.ndarray):
    tp = np.sum((y_pred == 1) & (y_true == 1))
    fp = np.sum((y_pred == 1) & (y_true == 0))
    tn = np.sum((y_pred == 0) & (y_true == 0))
    fn = np.sum((y_pred == 0) & (y_true == 1))
    acc = (tp + tn) / (tp + tn + fp + fn)
    pre = tp / (tp + fp)
    rec = tp / (tp + fn)
    f1 = 2 * pre * rec / (pre + rec)
    return {'accuracy': acc, 'precision': pre, 'recall': rec, 'f1': f1}


# X_train, X_test, y_train, y_test = get_data('iris_2_classes.csv', train_size=0.8)
X_train, X_test, y_train, y_test = get_data('iris_2_classes_fixed.csv', train_size=0.85)

model = KNN(k=10)
model.fit(X_train, y_train)
pred = model.predict(X_test)
# print(pred)

labels = np.argmax(pred, axis=1)

print(get_metrics(labels, y_test))
