# getmatvar

This is a multithreaded C-based MEX function for fast extraction of variables from MATLAB 7.3+ HDF5 format MAT-files with support for all MATLAB fundamental types except for function handles, tables, and class objects. The function aims to extend the functionality of `load` and improve overall performance.

There is a prebuilt binary available for Windows, and a tar of the sources in [Releases](https://github.com/gharveymn/getmatvar/releases). If building sources, running `INSTALL.m` will build it for you and output to the `bin` folder.

Be aware that I have not tested everything, so please do not use this for sensitive applications. If you do find a bug, feel free to open an issue or email me at [gharveymn@gmail.com](mailto:gharveymn@gmail.com).

## General Usage
The MEX function is run with a MATLAB function as an entry-point for easy extraction of variables directly into the workspace in the style of `load` (the core function `getmatvar_.c` returns a single struct).

```matlab
Usage:  
	getmatvar(filename)
	var = getmatvar(filename,'var')
	[var1,...,varN] = getmatvar(filename,'var1',...,'varN')
		
	getmatvar(__,'-t',n) specifies the number of threads to use
	with the next argument n.
	getmatvar(__,'-st') restricts getmatvar to a single thread.
	getmatvar(__,'-sw') suppresses warnings from getmatvar.

Example:
	>> getmatvar('my_workspace.mat'); 			  % All variables
	>> my_struct = getmatvar('my_workspace.mat', 'my_struct') % Only variable my_struct
```

While the normal `load` function can extract individual variables, it cannot handle nested variables---`getmatvar` allows this functionality. This is convenient if one needs to check only one sub-variable of a large struct.

```matlab
Example:
	>> my_struct.my_array = rand(100,101,102);
	>> my_struct.my_substruct.flags = {true,false,false};
	>> my_struct.my_substruct.data = magic(100);
	>> save('my_workspace.mat','my_struct');
	>> flag = getmatvar('my_workspace.mat','my_struct.my_substruct.flags{3}');
	>> flag
	flag =
  	  logical
	   0

```

## Build
In addition to the standard MEX build, there are also several ways to compile the standalone debug build.

### MATLAB Native
Running `INSTALL.m` will build the MEX function with the default MATLAB MEX compiler. This function is written for the C99 standard, so it will not compile on older distributions of MSVC or LCC.

### CMake
There are `CMakeLists` files configured for the non-MEX debug build and for a pseudo-build which will not build `mex`, but do set up the proper paths for symbol indexing.

### Make
The Make file is set up to compile the debug build on Unix systems.

## Known Issues

- MATLAB objects are not supported. This may be implemented at some time in the future, but it is not a high priority.
- A smaller amount of testing has been done on Linux than on Windows. There has been no testing done on 32-bit Windows, and very little done on 32-bit Linux. No testing has been done on MacOS, but it should be covered by Linux testing.
- Variable selection does not support individual indices of arrays (yet).

## Acknowledgements

Credit to the original author(s) of `mman-win32` for their Windows implementation of `mman`, Eric Biggers for `libdeflate`, and to Courtney Bonner for providing much of the original framework/foundation for the project and for her general assistance.