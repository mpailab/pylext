import sys
import platform
from setuptools import find_packages
from skbuild import setup


extra_cmake_args = ['-DBUILD_PYLEXT=ON']

setup(
    packages=find_packages(),
    package_data={'pylext': ['macros/*.pyg']},
    cmake_args=extra_cmake_args,
)
