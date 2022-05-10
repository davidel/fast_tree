import fast_tree_pylib as pft

import numpy as np
import math
import os
import pickle


class SklForest(object):

  def __init__(self, num_trees, **kwargs):
    self._num_trees = num_trees
    self._args = kwargs.copy()
    self._forest = None

  def __len__(self):
    return len(self._forest) if self._forest is not None else 0

  def __str__(self):
    if self._forest is None:
      return None

    return self._forest.str(precision=self._args.get('precision', 10))

  def fit(self, X, y):
    if X.dtype != np.float32:
      X = X.astype(np.float32)
    if y.dtype != np.float32:
      y = y.astype(np.float32)

    cols = []
    for i in range(0, X.shape[1]):
      cols.append(X[:, i])

    self._forest = pft.create_forest(cols, y, self._num_trees,
                                     opts=self._args,
                                     seed=self._args.get('seed', 31871551),
                                     num_threads=self._args.get('num_threads', 0))

    return self

  def predict(self, X):
    evres = self._forest.eval(X)

    result = np.empty((len(X),), dtype=X.dtype)
    for i, rt in enumerate(evres):
      result[i] = np.mean(rt)

    return result

  def __getstate__(self):
    state = self.__dict__.copy()
    state['__forest_str'] = str(self)
    del state['_forest']

    return state

  def __setstate__(self, state):
    forest_str = state.pop('__forest_str', None)
    self.__dict__.update(state)
    self._forest = pft.load_forest(forest_str) if forest_str is not None else None
