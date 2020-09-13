from Cython.Distutils import build_ext
from setuptools import Extension, setup, find_packages
from Cython.Build import cythonize

ext_modules = [
    Extension(
        name = 'cfgexts.parser._wrap',
        sources = [
            'cfgexts/parser/_wrap.pyx', 
            'cfgexts/parser/src/apply.cpp', 
            'cfgexts/parser/src/Exception.cpp', 
            'cfgexts/parser/src/PackratParser.cpp', 
            'cfgexts/parser/src/Parser.cpp'
        ],
        include_dirs=[
            'D:/projects/cogen/ctest/cfgexts/parser', 
            'D:/projects/cogen/ctest/cfgexts/parser/src'
        ], 
        language='c++',
        extra_compile_args=['/std:c++17', '/O2', '/arch:AVX2'],
    ),
]

ext_modules = cythonize(ext_modules)

setup(
    name='cfgexts',
    version='0.0.1',
    author='Example Author',
    author_email='author@example.com',
    description='Context-free grammars in python',
    long_description='long_description',
    long_description_content_type='text/markdown',
    url='https://github.com/mpailab/CFG-parser',
    packages=find_packages(),
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
    ext_modules=ext_modules,
    package_data={"cfgexts.parser": ["_wrap.pyx"]},
)