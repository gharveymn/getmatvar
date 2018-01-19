addpath('tests');
addpath('bin');
clear;

rng('shuffle')
numtests = 1000;
lents = 0;

maxDepth = 3;
maxElements = 500000;
ignoreUnusables = true;

doCompare = true;

for i = 1:numtests
	if(mod(i,2) == 1)
		test_struct1 = randVarGen(maxDepth, maxElements, ignoreUnusables);
		save('res/test_struct1.mat', 'test_struct1');
		clear('test_struct1');
		getmatvar('res/test_struct1.mat','-sw');
		
		if(doCompare)
			gmvtest_struct1 = test_struct1;
			clear('test_struct1');
			load('res/test_struct1.mat');
			[similarity,gmv,ld] = compstruct(gmvtest_struct1, test_struct1);
			if(~isempty(gmv) || ~isempty(ld))
				diffs = find(gmv ~= ld);
				mindiffind = min(find(gmv ~= ld));
				error('getmatvar failed to load correctly on test_struct1')
			end
			clear('gmvtest_struct1');
		end
			
		clear('test_struct1');
		if(i > 2)
			delete('res/test_struct2.mat');
		end
		
	else
		test_struct2 = randVarGen(maxDepth, maxElements, ignoreUnusables);
		save('res/test_struct2.mat', 'test_struct2');
		clear('test_struct2');
		getmatvar('res/test_struct2.mat','-sw');
		
		if(doCompare)
			gmvtest_struct2 = test_struct2;
			clear('test_struct2');
			load('res/test_struct2.mat');
			[similarity,gmv,ld] = compstruct(gmvtest_struct2, test_struct2);
			if(~isempty(gmv) || ~isempty(ld))
				diffs = find(gmv ~= ld);
				mindiffind = min(find(gmv ~= ld));
				error('getmatvar failed to load correctly on test_struct2')
			end
			clear('gmvtest_struct2');
		end
		
		clear('test_struct2');
		delete('res/test_struct1.mat');
	end
	timestr = sprintf('%d/%d',i,numtests);
	fprintf([repmat('\b',1,lents) timestr]);
	lents = numel(timestr);
end
fprintf('\n');