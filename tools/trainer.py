import argparse
import numpy as np
import os
import pickle
import pandas as pd
import py_fast_tree as pft
import re


def _load_dataframe(path, csv_args=None):
  ext = os.path.splitext(os.path.basename(path))[1].lower()
  if ext == '.pkl':
    return pd.read_pickle(path)
  elif ext == '.csv':
    csv_args = csv_args or dict()
    return pd.read_csv(path, **csv_args)
  else:
    raise RuntimeError(f'Unknown extension "{ext}" for file {path}')


def _get_csv_args(args):
  index_col = args.index_col
  if index_col is not None:
    if index_col.lower() == 'false':
      index_col = False
    elif isinstance(index_col, int):
      index_col = int(index_col)

  return dict(dtype=np.dtype(args.dtype), index_col=index_col)


def _get_numeric(value):
  if value is not None and not isinstance(value, (int, float)):
    if re.match(r'\d+$', value):
      value = int(value)
    else:
      try:
        value = float(value)
      except:
        pass

  return value


def _get_forest_options(args):
  return dict(
    seed=args.seed,
    num_trees=args.num_trees,
    num_threads=args.num_threads,
    precision=args.precision,
    max_rows=_get_numeric(args.max_rows),
    max_columns=_get_numeric(args.max_columns),
    min_leaf_size=args.min_leaf_size,
    num_split_points=args.num_split_points,
    min_split_error=args.min_split_error,
    same_eps=args.same_eps)


def _get_train_test_indices(nrows, base, size, gap=0):
  assert base >= 0.0 and base < 1.0, f'base = {base}'
  assert size > 0.0 and size < 1.0, f'size = {size}'
  if base + size > 1.0:
    base = 1.0 - size
  ibase = int(nrows * base)
  isize = int(nrows * size)

  test_indices = np.full((nrows, ), False)
  test_indices[ibase: ibase + isize] = True

  train_indices = ~ test_indices

  if gap > 0:
    test_indices[ibase: ibase + gap] = False

  return train_indices, test_indices


def _train_slice(X, y, times, ft_opts,
                 base=0.9,
                 size=0.1,
                 gap=0,
                 threshold=0.5,
                 output_file=None):
  train_indices, test_indices = _get_train_test_indices(len(X), base, size, gap=gap)

  X_train, y_train = X[train_indices], y[train_indices]
  X_test, y_test = X[test_indices], y[test_indices]

  sft = pft.SklForest(**ft_opts)

  sft.fit(X_train, y_train)

  y_ = sft.predict(X_test)

  y_mask = y_ > threshold
  y_test_mask = y_test > threshold
  one_match = (y_mask * y_test_mask).sum() * 100.0 / y_mask.size
  match = (y_mask == y_test_mask).sum() * 100.0 / y_mask.size
  times_ = times[y_mask]

  if output_file:
    with open(output_file, mode='wb') as f:
      pickle.dump(sft, f)

  return pft.Obj(one_match=one_match, match=match, buy_times=times_, forest=sft)


def _write_times(times_file, buy_times):
  with open(times_file, mode='w') as tf:
    tf.write('buy_times\n')
    for t in buy_times:
      tf.write(f'{t:.3f}\n')


def _test(args, X, y, times):
  ft_opts = _get_forest_options(args)

  test_steps = args.test_steps
  if test_steps is None:
    test_steps = int(round((1.0 - args.test_base) / args.test_size))

  buy_times = []

  base = args.test_base
  for _ in range(0, test_steps):
    size = min(args.test_size, 1.0 - base)
    if size < 1e-4:
      break

    sres = _train_slice(X, y, times, ft_opts,
                        base=base,
                        size=size,
                        gap=args.test_gap,
                        threshold=args.test_threshold)

    print(f'BASE = {base:.2f}\tONEM = {sres.one_match:.3f}%\tPREC = {sres.match:.2f}%')

    buy_times.append(sres.buy_times)
    base += size

  buy_times = np.sort(np.concatenate(buy_times))

  if args.times_file:
    _write_times(args.times_file, buy_times)


def _main(args):
  csv_args = _get_csv_args(args)

  Xdf = _load_dataframe(args.input_file, csv_args=csv_args)
  Ydf = _load_dataframe(args.target_file, csv_args=csv_args)

  times = Xdf.index.to_numpy(dtype=np.dtype(args.times_dtype))

  X = Xdf.to_numpy(dtype=np.dtype(args.dtype))
  y = Ydf.to_numpy(dtype=np.dtype(args.dtype))
  if y.ndim > 1:
    y = np.squeeze(y, axis=1)

  if args.slice:
    sbase, send = [float(x) for x in args.slice.split(',')]
    sbase = int(len(X) * sbase)
    send = int(len(X) * send)

    X = X[sbase: send]
    y = y[sbase: send]
    times = times[sbase: send]

  _test(args, X, y, times)


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Trains And Eval Using FastTree',
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('--input_file', type=str, required=True,
                      help='The path to the input file containing the training data')
  parser.add_argument('--target_file', type=str, required=True,
                      help='The path to the input file containing the training target')

  parser.add_argument('--slice', type=str,
                      help='The BASE,END slicing for the input data')

  parser.add_argument('--dtype', type=str, default='float32',
                      help='The type for the input data')
  parser.add_argument('--index_col', type=str,
                      help='The index column ordinal then reading CSV files')
  parser.add_argument('--times_dtype', type=str, default='float64',
                      help='The type for the index times (EPOCH timestamp)')

  parser.add_argument('--seed', type=int,
                      help='The seed for the random number generator')
  parser.add_argument('--num_trees', type=int,
                      help='The number of trees in the forest')
  parser.add_argument('--num_threads', type=int,
                      help='The number of threads to use for the forest generation')
  parser.add_argument('--precision', type=int,
                      help='The number of digits to use to save the forest')
  parser.add_argument('--max_rows', type=str,
                      help='The maximum number of rows to be used to build each tree')
  parser.add_argument('--max_columns', type=str,
                      help='The maximum number of columns to be used to build each tree')
  parser.add_argument('--min_leaf_size', type=int,
                      help='The minimum number of values within a leaf')
  parser.add_argument('--num_split_points', type=int,
                      help='The number of random split point to use to search for the best split')
  parser.add_argument('--min_split_error', type=float,
                      help='The minimum split error improvement for a split to be considered')
  parser.add_argument('--same_eps', type=float,
                      help='The epsilon to be used to consider two values to be the same')

  parser.add_argument('--test_threshold', type=float, default=0.5,
                      help='The threshold to be used to classify buy triggers')
  parser.add_argument('--test_base', type=float, default=0.0,
                      help='The base of the test data (0..1)')
  parser.add_argument('--test_size', type=float, default=0.1,
                      help='The base of the test data (0..1)')
  parser.add_argument('--test_steps', type=int,
                      help='The number of test steps from --test_base with --test_size increments')
  parser.add_argument('--test_gap', type=int, default=0,
                      help='The number of test records to skip (at the beginning) to avoid testing on trained samples')
  parser.add_argument('--times_file', type=str,
                      help='The path to the output times CSV file')

  args = parser.parse_args()

  _main(args)
