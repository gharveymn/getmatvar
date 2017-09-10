addpath('tests')
addpath('src')
cd src

try
	
% 	mex '-LC:\Anaconda3\Library\lib'  ...
% 		-lz -O -v CFLAGS="$CFLAGS -std=c11" getmatvar_.c...
% 		extlib/mman-win32/mman.c...
% 		fileHelper.c...
% 		getPageSize.c...
% 		mapping.c...
% 		numberHelper.c...
% 		queue.c...
% 		chunkedData.c...
% 		mexPointerSetters.c...
% 		readMessage.c
	
	libdeflate_path = ['-L' pwd '\extlib\libdeflate'];
	mex(libdeflate_path, '-llibdeflate', '-g', '-v', 'CFLAGS="$CFLAGS -std=c11"',... 
		'getmatvar_.c',...
		'extlib/mman-win32/mman.c',...
		'fileHelper.c',...
		'getPageSize.c',...
		'mapping.c',...
		'numberHelper.c',...
		'queue.c',...
		'chunkedData.c',...
		'mexPointerSetters.c',...
		'readMessage.c')

	cd ..

catch ME
	
	cd ..
	rethrow(ME)
	
end