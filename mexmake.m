addpath('src')
output_path = fullfile(pwd,'bin');

try
	
	if(exist('doINSTALL','var'))
		mexflags = {'-O', '-silent', '-outdir', output_path};
	else
		mexflags = {'-g', '-v', '-outdir', output_path};
	end
	
	[comp,maxsz,endi] = computer;
	libdeflate_dir = fullfile(pwd,'src','extlib','libdeflate');
	
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
	
	for i = 1:numel(sources)
		sources{i} = fullfile(pwd,'src',sources{i});
	end
	
	if(ispc)
		
		if(verLessThan('matlab','8.3'))
			fprintf('-Setting up the mex compiler...')
			try
				copyfile(fullfile(prefdir,'mexopts.bat'),fullfile(pwd,'mexopts.bat'),'f');
				mexopts_cont = strrep(fileread('mexopts.bat'),'set COMPFLAGS=','set COMPFLAGS=-std=c99 ');
				mexopts_fid = fopen(fullfile(pwd,'mexopts.bat'),'w');
				fprintf(mexopts_fid, '%s', mexopts_cont);
				fclose(mexopts_fid);
				clear mexopts_cont mexopts_fid
			catch ME2
				fprintf(' failed!\n')
				rethrow(ME2);
			end
			fprintf(' successful.\n')
			mexflags = [mexflags, {'-f', fullfile(pwd,'mexopts.bat')}];
		else
			mexflags = [mexflags {'CFLAGS="$CFLAGS -std=c99"'}];
		end
		
		if(maxsz > 2^31-1)
			libdeflate_dir = fullfile(libdeflate_dir,'win','x64');
			mexflags = [mexflags {'-largeArrayDims'}];
		else
			libdeflate_dir = fullfile(libdeflate_dir,'win','x86');
			mexflags = [mexflags {'-compatibleArrayDims', '-DMATLAB_32BIT'}];
		end
		
		sources = [sources,...
			{fullfile(pwd,'src','extlib','mman-win32','mman.c'),...
			fullfile(libdeflate_dir,'libdeflate.lib')}
			];
		
	elseif(isunix || ismac)
		
		warning('off','MATLAB:mex:GccVersion_link');
		
		if(verLessThan('matlab','8.3'))
			fprintf('-Setting up the mex compiler...')
			try
				copyfile(fullfile(prefdir,'mexopts.sh'),fullfile(pwd,'mexopts.sh'),'f');
				mexopts_cont = strrep(fileread('mexopts.sh'),'-ansi','-std=c99');
				mexopts_fid = fopen(fullfile(pwd,'mexopts.sh'),'w');
				fprintf(mexopts_fid, '%s', mexopts_cont);
				fclose(mexopts_fid);
				clear mexopts_cont mexopts_fid
			catch ME2
				fprintf(' failed!\n')
				rethrow(ME2);
			end
			fprintf(' successful.\n')
			mexflags = [mexflags, {'-f', fullfile(pwd,'mexopts.sh')}];
		else
			mexflags = [mexflags {'CFLAGS="$CFLAGS -std=c99"'}];
		end
		
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
	fprintf(' successful.\n%s\n',['-The function is located in ' fullfile(pwd,'bin') '.'])
	
	rmpath('src');
	addpath('bin');
	clear mexflags sources libdeflate_dir output_path comp endi maxsz doINSTALL
	
catch ME
	
	rethrow(ME)
	
end