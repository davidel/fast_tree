from setuptools import setup, Extension

VERSION = '0.1'

setup(
    name='py_fast_tree',
    version=VERSION,
    packages=['py_fast_tree'],
    package_dir={'py_fast_tree': '.'},
    package_data={'py_fast_tree': ['py_fast_tree.cpython-39-darwin.so']},
)
