import glob
import os
import subprocess

from setuptools import setup, Extension


VERSION = '0.1'

PRJ_PATH = os.path.dirname(os.path.abspath(__file__))


def cmake_build():
    subprocess.check_call(['cmake', 'CMakeLists.txt'])
    subprocess.check_call(['make'])


def get_module_libs(prj_path):
    return [os.path.basename(x) for x in glob.glob(os.path.join(prj_path, 'py_fast_tree.cpython-*.so'))]


cmake_build()


setup(
    name='py_fast_tree',
    version=VERSION,
    packages=['py_fast_tree'],
    package_dir={'py_fast_tree': '.'},
    package_data={
        'py_fast_tree': get_module_libs(PRJ_PATH),
    },
)