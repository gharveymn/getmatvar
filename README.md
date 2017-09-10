# getmatvar

This is a C-based MEX function for fast extraction of variables from Matfiles with support for all MATLAB fundamental types except for tables and function handles, which do not have mex function support.

A prebuild binary for 64-bit Windows is available in the `bin` directory, however if you're running on 32-bit you will have to build it manually.

There are some test functions and matfiles to experiment with if you want to do some benchmarking.

## General Usage
The MEX function is run with a Matlab function as an entry-point for easy extraction of variables (since `getmatvar_.c` returns a single struct).

```matlab
Usage:  
	getmatvar(filename,variable)
	getmatvar(filename,variable1,...,variableN)
		
	filename	a character vector of the name of the file with a .mat extension
	variable	a character vector of the variable to extract from the file

Example:
	getmatvar('my_workspace.mat', 'my_struct')
```

## Build with CMake

The batch scripts named `build_with_msvc.bat` and `build_with_mingw.bat` will build with their titular compilers (given you have all the prerequisite executables). If compiling with MSVC you'll have to open up the solution and build the `INSTALL` module (As well as all the others. It just isn't selected at first.) to output to `bin`. You won't have to do anything else for the MinGW build. I haven't tested this with 32-bit Windows since I don't have a copy, however systems are in place to deal with that matter, so it should follow the same procedure.

Indeed this fork is for Windows, but it should be pretty easy to set up for compilation in *nix. I've tried to make the source as system-friendly as possible, but I may have missed some things. Try using running `mexmake.m` to compile with `mex` before changing anything. 

### Acknowledgements

Credit to the original author(s) of `mman-win32` for their Windows implementation of `mman` and to Eric Biggers for `libdeflate`, as well as Earnie Boyd for the `param.h` header. Thanks for Courtney Bonner for providing much of the framework for the project and for her general assistance.