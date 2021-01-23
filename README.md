# MutantBench: an Equivalent Mutant Problem Benchmarking Framework
**By**: Lars van Hijfte


Mutation testing is a method of evaluating the effectiveness of a test suite. It does this by altering the program with a predefined set of operations, resulting in a __mutant__. The effectiveness of the test suite is then determined by the amount of mutants the test suite correctly identifies to be altered. However, these alternations can sometimes lead to semantically equivalent programs, called __equivalent mutants__. These mutants can never be detected by a test suite, and thus incorrectly lowering the effectiveness of the test suite. The problem of these equivalent mutants is called the Equivalent Mutant Problem, or EMP.

Research in the equivalent mutant problem is still ongoing that since let to the creation of multiple reusable mutant datasets. These datasets can be used to measure the performance of a solution, or compare multiple solutions by using the same dataset.

This repository presents MutantBench, a novel open-source benchmarking framework that is designed with a focus on FAIR data principles and adoptability by the community. For this, it makes four contributions.

First, we design a [dataset standards](mutantbench/standard.ttl) which aims to make datasets that follow this standard findable, accessible, interoperable, and reusable.

Then we also present a [converter](mutantbench/convert.py) existing datasets to this new standard.

After which we combine these to this new standard to create a [mutant dataset](mutantbench/dataset.ttl) containing 4400 mutants with 1416 equivalent mutants.

Lastly, we create a [benchmarking tool](mutantbench/benchmark.py) that uses this combined dataset to give a detailed report on the performance of any tool trying to solve the equivalent mutant problem.

## Usage

### Converter

The converter automatically converts directories of mutants to the new dataset. For this, the functions `get_program_locations` and
`get_mutant_locations` needs to be implemented as described in the docstrings. Then, running the converter required the following arguments:
```
source: a reference to the paper where the mutants originate from
language: the language of the dataset
directory: the directory of the dataset
out_dir: the directory you want the programs to be generated in
```

### Benchmark


Example usage: `python3 benchmark.py bash examples/DEMInterface/interface_dem.sh DEM c --programs bubble Calendar Calender Day Defroster Tcas Triangle`

The benchmark allows is suited for all three Equivalent Mutant Problem solution types:
Suggesting Equivalent Mutants (SEM), Detecting Equivalent Mutants (DEM), and Avoiding Equivalent Mutant Generation (AEMG). Each of these solutions have a different interface to communicate with.

#### DEM


For DEM tools, `MBDetectMutants` is called. This function is expected to create a file containing a list of all the equivalent mutants and return the path to this file. Example output file:
```
d9a793087ae22fea2e49c17724f85f978f1692a0
dcd93b222fb338f336d5034aa529980a09ee2489
...
ed6891fd2ec26df6b370ea43a97405b8b0ed1850
fa0e70aeb66713093be9ad9ee319c90013eae492
```

#### SEM

For SEM tools, `MBSuggestMutants` is called. This function is expected to create a file containing a list of all mutants with the probability of equivalency attached (`[mutant_uri], [probability]`) and return the path to this file. Example output file:
```
9901687aab6fad45994b7141d6aa2d8e8dd2d8f3, 1
a40c1bf3e58f41eacf09c9b74132230adc04e4e7, 0.634
...
b9954e7cdf94276ee97b0429da5cd95240543282, 0.8
baf09fd4032f7e64e08bc28c6a8e3b0715f28afd, 0.23
```

#### SEM

For AEMG tools, `MBAvoidEquivalentMutants` is called. This function should generate mutants in the same structure as MutantBench would generate mutants for DEM/SEM tools and return the path to this directory. Example output directory:
```
/
|__ Triangle
|  |__ original.java
|  |__ mutants
|     |__ 435298763.java
|     |__ 677345678.java
|     |__ 918235623.java
|__ Min
   |__ original.java
   |__ mutants
      |__ 243562345.java
      |__ 576841234.java
```


#### Bash

Communicating via Bash is done through the `BashInterface` interface.
Create a file where the first argument is the function to run (e.g. `MBSuggestMutants`) and the second argument is a string containing the location of the generated programs and possibly mutants.
An example of this can be found in [interface_dem.sh](examples/DEMInterface/interface_dem.sh).

#### Java

Communicating via Java is done through the `JavaInterface` interface. This makes use of [Py4J](https://www.py4j.org/).
To communicate via Java, you first need to start the Gateway:
```bash
cd examples/JavaInterface
javac -cp ".:py4j0.10.9.1.jar:" MBInterface.java && java -cp ".:py4j0.10.9.1.jar:" MBInterface
```
After this, the benchmark can be run, which will then comminicate with this gateway.
An example of this can be found in [MBInterface.java](examples/JavaInterface/MBInterface.java).


#### C

Communicating via C is done though the `CInterface` interface.

The C interface uses the Python library [ctypes](https://docs.python.org/3/library/ctypes.html)
to communicate with a shared C library which can be created using the following GCC command: `gcc -shared libTool.so tool.c`.
The parameter and return type both need to be a [char pointer type](https://docs.python.org/3/library/ctypes.html\#ctypes.c_char_p).

The resulting command shared library can then be accessed by Python without any gateway as is required for Java. Full example:
```
gcc -o examples/CInterface/interface.so -shared -fPIC -O2 examples/CInterface/interface.c
python3 benchmark.py c examples/CInterface/interface.so DEM c
```