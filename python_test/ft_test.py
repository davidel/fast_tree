import numpy as np
import math
import os
import pickle
import py_fast_tree as pft
import tempfile
import unittest


class _dict(object):

  def __init__(self, **kwargs):
    for k, v in kwargs.items():
      setattr(self, k, v)



def _rand_data(nrows, ncols, dtype=np.float32):
  c = [np.random.rand(nrows).astype(dtype) for _ in range(0, ncols)]
  t = np.random.rand(nrows).astype(dtype)

  return _dict(columns=c, target=t)


def _gen_ramp(space, count, inverse=False, dtype=np.float32):
  data = np.empty((space * count,), dtype=dtype)

  if inverse:
    gen = range(count - 1, -1, -1)
  else:
    gen = range(0, count, 1)

  i = 0
  for v in gen:
    for _ in range(0, space):
      data[i] = v
      i += 1

  return data


def _make_forest(nrows, ncols, ntrees, opts=None):
  if opts is None:
    opts = dict(max_rows=0.75, max_columns=int(math.sqrt(ncols)) + 1)

  rd = _rand_data(nrows, ncols)

  return pft.create_forest(rd.columns, rd.target, ntrees, opts=opts)



class TestForest(unittest.TestCase):

  def test_create(self):
    N = 240
    C = 10
    T = 8

    ft = _make_forest(N, C, T)

    self.assertEqual(len(ft), T)

  def test_simple(self):
    S = 10
    C = 12
    T = 1
    COLUMNS = [
      _gen_ramp(S, C),
    ]
    TARGET = _gen_ramp(10, 12)
    OPTS = dict()

    ft = pft.create_forest(COLUMNS, TARGET, T, opts=OPTS)

    for n in range(0, C):
      y = ft.eval(np.array([[n]], dtype=np.float32))

      for a in y[0]:
        self.assertEqual(a, n)

  def test_str(self):
    N = 240
    C = 10
    T = 4

    ft = _make_forest(N, C, T)

    s = ft.str(precision=10)

    self.assertGreater(len(s), 0)

  def test_load(self):
    N = 240
    C = 10
    T = 4

    ft = _make_forest(N, C, T)

    s = ft.str(precision=10)
    lft = pft.load_forest(s)

    self.assertEqual(len(lft), T)

    row = np.random.rand(1, C).astype(np.float32)
    y = ft.eval(row)
    ly = lft.eval(row)

    for e, le in zip(*y, *ly):
      self.assertTrue(np.allclose(e, le))

  def test_load_from_file(self):
    N = 240
    C = 10
    T = 4

    ft = _make_forest(N, C, T)

    with tempfile.TemporaryDirectory() as tmpdir:
      fname = os.path.join(tmpdir, 'forest.txt')
      with open(fname, mode='w') as f:
        s = ft.str(precision=10)
        f.write(s)

      lft = pft.load_forest_from_file(fname)

      self.assertEqual(len(lft), T)

      row = np.random.rand(1, C).astype(np.float32)
      y = ft.eval(row)
      ly = lft.eval(row)

      for e, le in zip(*y, *ly):
        self.assertTrue(np.allclose(e, le))

  def test_skl_forest(self):
    N = 500
    C = 10
    T = 4

    sft = pft.SklForest(
      num_trees=T,
      max_rows=0.75,
      max_columns=int(math.sqrt(C)) + 1,
      seed=31455907,
    )

    X = np.random.rand(N, C).astype(np.float32)
    y = np.sum(X, axis=1)

    sft.fit(X, y)

    self.assertEqual(len(sft), T)

    y_ = sft.predict(X)

    er = np.abs(y - y_).sum() / np.abs(y).sum()
    self.assertLess(er, 0.1)

    ps = pickle.dumps(sft)
    psft = pickle.loads(ps)

    self.assertEqual(len(sft), len(psft))

    py_ = psft.predict(X)

    self.assertTrue(np.allclose(y_, py_))


if __name__ == '__main__':
  unittest.main()
