import platform
from setuptools import find_packages
from skbuild import setup

cmake_args = ['-DBUILD_PYLEXT=ON']
if platform.system() == "Windows":
    cmake_args.append('-GVisual Studio 16 2019')

setup(
    name="pylext",
    version="0.1.0",
    author="Bokov G.V., Kalachev G.V.",
    author_email='author@example.com',
    description="Domain specific language extensions for python",
    long_description='long_description',
    long_description_content_type='text/markdown',
    url='https://github.com/mpailab/CFG-parser',
    license="MIT",
    classifiers=[
        'Intended Audience :: Science/Research',
        'Intended Audience :: Developers',
        'Topic :: Software Development',
        'Topic :: Scientific',
        'Operating System :: Microsoft :: Windows',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'License :: OSI Approved :: MIT License',
    ],
    python_requires='>=3.6',
    install_requires=['cython', 'scikit-build', 'setuptools'],
    packages=find_packages(),
    cmake_args=cmake_args,
)