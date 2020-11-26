import os
from setuptools import setup, find_packages
import _version as version
thisdir = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(thisdir, "README.md"), "r") as fh:
    long_description = fh.read()

description = 'Benchmarking tool for the Equivalent Mutant Problem'

setup(
    name=version._framework_name,
    python_requires='>3.7.0',
    version=version.__version__,
    description=description,
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='Lars van Hijfte',
    url='git@github.com:Larspolo/MutantBench.git',
    packages = ['mutantbench'],
    py_modules = ['_version'],
    include_package_data=True,
    install_requires = [
        'ctypes',
        'py4j',
        'sqlalchemy',
        # "flask",
    ],
    entry_points={
        'console_scripts': [
            'mutantbench=mutantbench:main',
        ],
    },

    classifiers=[
        'Development Status :: 3 - Alpha',
        'Natural Language :: English',
        'Intended Audience :: Science/Research',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Testing',
        'License :: GNU General Public License v3.0',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3',
    ]
)
