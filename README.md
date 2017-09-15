# getmatvar

This is a multithreaded C-based MEX function for fast extraction of variables from MATLAB 7.3+ HDF5 format Matfiles with support for all MATLAB fundamental types except for tables and function handles, which do not have MEX library support.

A prebuilt binary for 64-bit Windows is available in the `bin` directory, however if you're running on 32-bit you will have to build it manually.

There are also some test functions and Matfiles to experiment with if you want to do some benchmarking.

## General Usage
The MEX function is run with a Matlab function as an entry-point for easy extraction of variables directly into the workspace in the style of `load` (the core function `getmatvar_.c` returns a single struct).

```matlab
Usage:  
	getmatvar(filename,variable)
	getmatvar(filename,variable1,...,variableN)
		
	filename	a character vector of the name of the file with a .mat extension
	variable	a character vector of the variable to extract from the file

Example:
	>> getmatvar('my_workspace.mat', 'my_struct');
```

If you find it more convenient, you can also invoke the MEX function directly. In this case there is an output of a single struct.

```matlab
Example:
	>> my_array = rand(100,101,102);
	>> save('my_workspace.mat','my_array');
	>> s = getmatvar_('my_workspace.mat', 'my_array');
	>> s
	s = 
  	  struct with fields:

	    my_array: [100×101×102 double]

```

## Build with CMake

The batch scripts named `build_with_msvc.bat` and `build_with_mingw.bat` will build with their titular compilers (given you have all the prerequisite executables). If compiling with MSVC you'll have to open up the solution and build the `INSTALL` module (As well as all the others. It just isn't selected at first.) to output to `bin`. You won't have to do anything else for the MinGW build. I haven't tested this with 32-bit Windows since I don't have a copy, however systems are in place to deal with that matter, so it should follow the same procedure.

Indeed this is Windows native, but it should be pretty easy to set up for compilation in *nix. I've tried to make the source as system-friendly as possible, but I may have missed some things. Try using running `mexmake.m` to compile with `mex` before changing anything. 

### Acknowledgements

Credit to the original author(s) of `mman-win32` for their Windows implementation of `mman`, Eric Biggers for `libdeflate`, Johan Hanssen Seferidis, for `threadpool`, as well as Earnie Boyd for the `param.h` header. Also a big thanks to Courtney Bonner for providing much of the framework/foundation for the project and for her general assistance.