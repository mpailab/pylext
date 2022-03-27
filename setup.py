import sys
import platform
from setuptools import find_packages
from skbuild import setup


extra_cmake_args = ['-DBUILD_PYLEXT=ON']
if platform.system() == "Windows":
    extra_cmake_args.append('-GVisual Studio 17 2022')


setup(
    packages=find_packages(),
    package_data={'pylext': ['macros/*.pyg']},
    cmake_args=extra_cmake_args,
)
