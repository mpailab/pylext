import sys
import platform
from setuptools import find_packages
from skbuild import setup


extra_cmake_args = ['-DBUILD_PYLEXT=ON']
if '--no-intrinsics' in sys.argv:
    extra_cmake_args.append('-DNO_INTRINSICS=ON')
    sys.argv.remove('--no-intrinsics')

setup(
    packages=find_packages(),
    package_data={'pylext': ['macros/*.pyg']},
    cmake_args=extra_cmake_args,
)
