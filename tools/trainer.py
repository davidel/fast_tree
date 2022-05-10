import argparse
import numpy as np
import os
import pandas as pd
import py_fast_tree as pft


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

  return dict(dtype=args.dtype, index_col=index_col)



def _main(args):
  csv_args = _get_csv_args(args)



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


  parser.add_argument('--time_index', action='store_true',
                      help='Assume the index is an EPOCH time, and convert it to DateTime')

  args = parser.parse_args()

  _main(args)
