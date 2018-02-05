addpath('src')
output_path = [pwd '/bin'];
cd src

try
	
	%mexflags = {'-O', '-silent', 'CFLAGS="$CFLAGS -Wall -std=c99"', '-outdir', output_path};
	mexflags = {'-g', '-v', '-outdir', output_path};
	
	[comp,maxsz,endi] = computer;
	libdeflate_dir = fullfile(pwd,'extlib','libdeflate');
	
	sources = {'getmatvar_.c',...
		'mexSet.c',...
		'cleanup.c',...
		'createDataObjects.c',...
		'ezq.c',...
		'mtezq.c',...
		'fillDataObjects.c',...
		'getDataObjects.c',...
		'getSystemInfo.c',...
		'init.c',...
		'navigate.c',...
		'numberHelper.c',...
		'placeChunkedData.c',...
		'placeData.c',...
		'readMessage.c',...
		'superblock.c',...
		'utils.c'};
	
	if(ispc)
		
		if(maxsz > 2^31-1)
			libdeflate_dir = fullfile(pwd,'extlib','libdeflate','win','x64');
			mexflags = [mexflags {'-largeArrayDims'}];
		else
			libdeflate_dir = fullfile(pwd,'extlib','libdeflate','win','x86');
			mexflags = [mexflags {'-compatibleArrayDims', '-DMATLAB_32BIT'}];
		end
		
		sources = [sources,...
			{fullfile(pwd,'extlib','mman-win32','mman.c'),...
			fullfile(libdeflate_dir,'libdeflate.lib')}
			];
		
	elseif(isunix || ismac)
		
		warning('off','MATLAB:mex:GccVersion_link');
		
		mex_setup_cmd = ['cp ' fullfile(prefdir,'mexopts.sh') ' ' fullfile(pwd,'mexopts.sh') ' ; '...
			'sed -i "s/-ansi/-Wall -std=c99/g" mexopts.sh'];
		
		fprintf('-Setting up the mex compiler...')
		[makestatus, cmdout] = system(mex_setup_cmd);
		clear mex_setup_cmd
		if(makestatus ~= 0)
			fprintf(' failed!\n')
			error(cmdout);
		end
		fprintf(' successful.\n')
		
		mexflags = [mexflags, {'-f', fullfile(pwd,'mexopts.sh')}];
		
		libdeflate_dir = fullfile(libdeflate_dir,'unix');
		if(maxsz > 2^31-1)
			mexflags = [mexflags {'-largeArrayDims'}];
		else
			mexflags = [mexflags {'-compatibleArrayDims', '-DMATLAB_32BIT'}];
		end
		
		terminal_cmd = ['cd ' libdeflate_dir ' ; '...
			'cd programs ; '...
			'chmod +x detect.sh ; '...
			'cd .. ; '...
			'make -e DECOMPRESSION_ONLY=TRUE libdeflate.a'];
		fprintf('-Compiling external libraries...')
		[makestatus, cmdout] = system(terminal_cmd);
		clear terminal_cmd
		if(makestatus ~= 0)
			fprintf(' failed!\n')
			error(cmdout);
		end
		fprintf(' successful.\n')
		%disp(cmdout)
		sources = [sources, {fullfile(libdeflate_dir,'libdeflate.a')}];
		
	end
	
	fprintf('-Compiling getmatvar...')
	mex(mexflags{:} , sources{:})
	fprintf(' successful.\n')
	
	cd ..
	rmpath('src');
	addpath('bin');
	clear mexflags sources libdeflate_dir output_path
	
catch ME
	
	cd ..
	rethrow(ME)
	
end