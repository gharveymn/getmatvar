# getmatvar

This is a multithreaded C-based MEX function for fast extraction of variables from MATLAB 7.3+ HDF5 format MAT-files with support for all MATLAB fundamental types except for function handles, tables, and class objects. These will be implemented in the near future.

Eventually there will be prebuilt binaries, but for now you'll have to build it yourself. Running `mexmake.m` will build it for you and output to the `bin` folder if you have all the needed dependencies. You might need to set some paths for the shared object libraries on Unix.

There are also some test functions and MAT-files to experiment with if you want to do some benchmarking.

## General Usage
The MEX function is run with a MATLAB function as an entry-point for easy extraction of variables directly into the workspace in the style of `load` (the core function `getmatvar_.c` returns a single struct).

```matlab
Usage:  
	getmatvar(filename,variable)
	getmatvar(filename,variable1,...,variableN)
	getmatvar(filename,variable1,...,variableN,option1,oparg1,...,optionM,opargM)
		
	filename	a character vector of the name of the file with a .mat extension
	variable	a character vector of the variable to extract from the 
	option		an option flag
	oparg		an option argument


	options:
		'-t(hreads)'			specify the number of threads to use, requires an integer oparg
		'-m(ultithread)'		specify whether to multithread, requires a boolean oparg
		'-suppress-warnings'/'-sw'	suppress warnings from the function, no oparg

Example:
	>> getmatvar('my_workspace.mat', 'my_struct');
```

If you find it more convenient, you can also invoke the MEX function directly. In this case there is an output of a single struct.

```matlab
Example:
	>> my_array = rand(100,101,102);
	>> save('my_workspace.mat','my_array');
	>> [s,~] = getmatvar_('my_workspace.mat', 'my_array');
	>> s
	s = 
  	  struct with fields:

	    my_array: [100×101×102 double]

```

## Build

There are several ways of building the library as a MEX function as well as a standalone debug build.

### MATLAB Native
Running `INSTALL.m` will build the MEX function with the MATLAB internal compiler. This is only set up for MinGW and MSVC on Windows, and GCC on Unix, so it may require some changes if using a different compiler. You may also need to set up paths to shared object files if compiling on Unix.

### CMake
The batch scripts named `build_with_msvc.bat` and `build_with_mingw.bat` will build the MEX function with their titular compilers (given you have all the prerequisite executables). If compiling with MSVC you'll have to open up the solution and build the `INSTALL` module (As well as all the others. It just isn't selected at first.) to output to `bin`. You won't have to do anything else for the MinGW build. I haven't tested this with 32-bit Windows since I don't have a copy, however systems are in place to deal with that matter, so it should follow the same procedure.

### Make

The Make file is set up to compile the debug build on Unix systems only at the moment.

### Known Issues

Indeed this is Windows native, but compilation on Unix shouldn't require too much troubleshooting --- I've tried to make the source as system-friendly as possible. Try using running `mexmake.m` to compile with `mex` before changing anything.

This library does not support MATLAB objects other than function handles as of yet. I have managed to reverse engineer their system, but I'm quite busy now, so that may be implemented by around December.

### Acknowledgements

Credit to the original author(s) of `mman-win32` for their Windows implementation of `mman`, Eric Biggers for `libdeflate`, and Johan Hanssen Seferidis for `threadpool`. Also a big thanks to Courtney Bonner for providing much of the framework/foundation for the project and for her general assistance.