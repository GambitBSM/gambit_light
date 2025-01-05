GAMBIT-light
============

**GAMBIT-light** is a powerful yet easy-to-use tool for computationally difficult statistical fits and optimisation tasks. The user can provide their target/likelihood function as a Python, C, C++ or Fortran library.

Some features of GAMBIT-light:

  - A unified interface to a collection of powerful, MPI-parallelised sampling/optimisation algorithms
  - Simple interface for connecting user-supplied target/likelihood functions (and sampling priors, if needed) as runtime plugins 
  - Outputs in different file formats (binary or text)
  - Systems for exception handling and logging
  - Safe shutdown and resuming of aborted runs
  - Run configuration via a simple YAML file

GAMBIT-light is a spin-off project from **GAMBIT** (the Global And Modular BSM Inference Tool), https://gambitbsm.org/, a software tool for large-scale statistical fits in particle physics and astrophysics. If you use GAMBIT-light, please cite the papers listed in the Citation section below.


Supported compilers and library dependencies
--

GAMBIT-light is built using the CMake system. The following libraries and packages must be installed prior to configuration:

**COMPULSORY**:

 - gcc >= 9 / llvm clang >= 10 / AppleClang >= 13 / icc >= 15.0.2
 - gfortran >= 9 / ifort >=15.0.2
 - Cmake 2.8.12 or greater
 - Python 3
 - Python modules: yaml, future, os, re, datetime, sys, getopt, shutil and itertools.
 - git
 - Boost 1.48 or greater
 - Eigen 3.1.0 or greater
 - GNU Scientific Library (GSL) 2.1 or greater
 - LAPACK
 - pkg-config

**OPTIONAL**:

 - HDF5 (for use of the hdf5 printer)
 - MPI (required for parallel sampling)
 - axel (speeds up downloads of scanners)
 - Python modules:
    - h5py (required to use hdf5 utilities located in gambit/Printers/scripts)


Building GAMBIT-light
--

GAMBIT-light itself is a C++ code. You build it once, and afterwards you connect your own target/likelihood function code to GAMBIT-light as a runtime plugin, in the form of a Python code or a C/C++/Fortran shared library. 

GAMBIT-light supports Linux and Mac OSX. (On Windows you can run it through WSL or Cygwin.) A full walkthrough of how to install all dependencies and build GAMBIT-light with AppleClang on OSX can be found in the file `README_OSX.md`.

Note that CMake might fail to find some dependencies on some systems without guidance. If you encounter problems configuring or building GAMBIT-light, have a look in `BUILD_OPTIONS.md` for a list of commonly used build options.

Here is a basic example of how to build GAMBIT-light and the full collection of "scanners" (sampling/optimisation libraries):

```console
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DEIGEN3_INCLUDE_DIR=/path/to/eigen -DWITH_MPI=On -DCMAKE_CXX_COMPILER=g++-11 -DCMAKE_C_COMPILER=gcc-11 -DCMAKE_Fortran_COMPILER=gfortran-11 ..
make -jN scanners   # where N is the number of cores to use for the build, e.g. 4
cmake ..            # this step is needed for GAMBIT to detect the built scanners
make -jN gambit
```

(If you prefer to turn off compiler optimisations, use `-DCMAKE_BUILD_TYPE=None` instead of `-DCMAKE_BUILD_TYPE=Release`.)

Instead of building all the scanners (`make -jN scanners`) you can build each scanner you need individually, e.g.

```console
make -jN diver
make -jN multinest
make -jN polychord
```

If for some reason you need to restart the build process, you can clean out the previous build as follows:

```console
cd build
make nuke-all
cd ..
rm -r build
```


Running GAMBIT-light
--

A GAMBIT-light run is configured with a single YAML file. A fully commented example is provided in `yaml_files/gambit_light_example.yaml`. You can run GAMBIT-light with this example configuration by doing

```console
./gambit -rf yaml_files/gambit_light_example.yaml
```

To see a complete list of command-line options, do 
```console
./gambit --help
```

When using an MPI-parallelised scanner, start GAMBIT-light with `mpiexec` or `mpirun`, e.g.

```console
mpiexec -np 4 ./gambit -rf yaml_files/your_configuration_file.yaml
```

for a run with 4 MPI processes.



Connect your own target/likelihood function
--

For step-by-step instructions on how to connect your own Python/C/C++/Fortran code to GAMBIT-light, see the README files and example codes in the 
`gambit_light_interface/example_*` directories.



Get GAMBIT plotting tools
--

To download a set of Python plotting tools for making 1D/2D profile likelihood and posterior plots from GAMBIT and GAMBIT-light output files, do:

```console
cd build
make get-gambit_plotting_tools
cd ..
```

The plotting tools will be downloaded to a folder `gambit_plotting_tools`. You can either run your plotting scripts from this folder or pip install the package: 

```console
pip install ./gambit_plotting_tools
```

Fully worked example from scan to plotting
--

This example performs a two-parameter scan that uses the (negative) Rosenbrock function as an example log-likelihood function for which we want to identify the best-fit point and map out the 1sigma and 2sigma confidence regions. The scan is performed using the Diver differential evolution scanner and should take around one minute when running with a single MPI process. The scan output is written to the directory `runs/gambit_light_example_rosenbrock_scan`.

- Run GAMBIT scan:

  ```console
  ./gambit -rf yaml_files/gambit_light_example_rosenbrock_scan.yaml
  ```

- Download the GAMBIT plotting tools (see above).

- Modify the beginning of the script `gambit_plotting_tools/example_gambit_light_2D_profile_like_hdf5.py` to point to your GAMBIT output file in `runs/gambit_light_example_rosenbrock_scan/samples/results.hdf5`.

- Make a 2D profile likelihood plot:

  ```console
  cd gambit_plotting_tools
  python example_gambit_light_2D_profile_like_hdf5.py
  ```
- To make a 1D profile likelihood plot, just look at the example `gambit_plotting_tools/example_1D_profile_like_hdf5.py` and make the appropriate changes. 

- If you change `use_scanner: diver` to e.g. `use_scanner: multinest` in the YAML file, choose a new output directory (change the `default_output_path` setting), and run GAMBIT-light again, you can also produce Bayesian posterior results. (Just look at the posterior example scripts `gambit_plotting_tools`.)

- At the end you should have plots looking e.g. like this: 
  
<<<<<<< HEAD
  <img src="example_plots/2D_profile__x1__x2__LogLike.png" alt="2D profile likelihood example plot" width="250"/>
  <img src="example_plots/1D_profile__x2__LogLike.png" alt="1D profile likelihood example plot" width="250"/>

  <img src="example_plots/2D_posterior__x1__x2.png" alt="2D posterior example plot" width="250"/>
  <img src="example_plots/1D_posterior__x2.png" alt="2D profile likelihood example plot" width="250"/>
=======
  <img src="example_results/2D_profile__x1__x2__LogLike.png" alt="2D profile likelihood example plot" width="400"/>
>>>>>>> 2c6d8ac3e33f37d5e8f1a7ba1af3d007e509dad3


Common issues
--

- **Multiple Python installations**: If you have multiple versions of Python installed on your system, or you are working with a Python virtual environment, it is recommended to manually set the paths to the specific Python library and header files that GAMBIT-light should use. This can be done via the flags `PYTHON_INCLUDE_DIR` and `PYTHON_LIBRARIES` in the cmake command, e.g. like this:
  ```console
  cmake -DPYTHON_INCLUDE_DIR=/usr/include/python3.10 -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.10.so (the rest of the cmake command) ..
  ```
  If needed, the `PYTHON_EXECUTABLE` flag can be used to ensure that GAMBIT-light also uses the correct Python executable, e.g. `-DPYTHON_EXECUTABLE=/usr/bin/python3`.

- **Multiple HDF5 libraries**: GAMBIT-light can fail to build or run due to conflicting versions of the HDF5 library. The most common source of this problem is that the Python package `h5py` has been built against an HDF5 version that differs from the version already installed on the system. One solution can be to reinstall `h5py` as follows:
  ```console
  pip3 uninstall h5py
  pip3 install --no-binary=h5py h5py
  ```
  If needed, you can specify the specific HDF5 version that h5py should be built against:
  ```console
  $ pip3 uninstall h5py
  $ HDF5_VERSION=1.10.7 pip3 install --no-binary=h5py h5py  
  ```
- **HDF5-related segmentation fault**: GAMBIT-light has two separate systems ("printers") for writing HDF5 output files. If using the YAML file option 
  ```yaml
  Printer:
    printer: hdf5
  ```
  leads to a segmentation fault (this issue is most commonly seen on Mac), then try switiching to the older HDF5 printer by using
  ```yaml
  Printer:
    printer: hdf5_v1
  ```

- **hdf5_v1 printer and the multinest scanner**: The multinest scanner currently cannot be used together with the hdf5_v1 printer.

- **gambit plotting tools throwing errors about axis syntax**: Much older versions of matplotlib may not allow the syntax used to create profile likelihood plots. This would require upgrading the matplotlib version.


Citation
--

Please cite the following GAMBIT papers if you use GAMBIT-light:

 - GAMBIT Collaboration: P. Athron, et al., **GAMBIT**: The Global and Modular Beyond-the-Standard-Model Inference Tool, Eur. Phys. J. C 77 (2017) 784, arXiv:1705.07908
 - GAMBIT Scanner Workgroup: G. D. Martinez, et al., Comparison of statistical sampling methods with **ScannerBit**, the GAMBIT scanning module, Eur. Phys. J. C 77 (2017) 761, arXiv:1705.07959

GAMBIT-light contains interfaces to several external codes, in particular optimisation or sampling tools. Please cite as appropriate when you use those codes.


Licensing
--

The BSD license below applies to all source files in the GAMBIT-light distribution.

License
--
Copyright (c) 2017-2024, The GAMBIT Collaboration
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
