import argparse
import numpy as np
import os
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
      value = float(value)

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


def _main(args):
  csv_args = _get_csv_args(args)

  Xdf = _load_dataframe(args.input_file, csv_args=csv_args)
  Ydf = _load_dataframe(args.target_file, csv_args=csv_args)

  times = Xdf.index.to_numpy(dtype=np.dtype(args.times_dtype))

  X = Xdf.to_numpy(dtype=np.dtype(args.dtype))
  y = Ydf.to_numpy(dtype=np.dtype(args.dtype))

  ft_opts = _get_forest_options(args)



if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Trains And Eval Using FastTree',
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('--input_file', type=str, required=True,
                      help='The path to the input file containing the training data')
  parser.add_argument('--target_file', type=str, required=True,
                      help='The path to the input file containing the training target')

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

  args = parser.parse_args()

  _main(args)
