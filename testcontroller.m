addpath('tests');
addpath('bin');
clear;

rng('shuffle')
numtests = 1000;
lents = 0;

maxDepth = 1;
maxElements = 50000;
ignoreUnusables = true;
stride = 100;
mvgavgtime = zeros(stride,1);
strideload = stride;
mvgavgtimeload = zeros(strideload,1);


doCompare = true;

for i = 1:numtests
	if(mod(i,2) == 1)
		test_struct1 = randVarGen(maxDepth, maxElements, ignoreUnusables);
		save('res/test_struct1.mat', 'test_struct1');
		clear('test_struct1');
		tic;
		getmatvar('res/test_struct1.mat','-sw');
		mvgavgtime(mod(i-1,stride)+1) = toc;
		if(doCompare)
			gmvtest_struct1 = test_struct1;
			clear('test_struct1');
			tic;
			load('res/test_struct1.mat');
			mvgavgtimeload(mod(i-1,strideload)+1) = toc;
			[similarity,gmv,ld] = compstruct(gmvtest_struct1, test_struct1);
			if(~isempty(gmv) || ~isempty(ld))
				%diffs = find(gmv ~= ld);
				%mindiffind = min(find(gmv ~= ld));
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
		tic;
		getmatvar('res/test_struct2.mat','-sw');
		mvgavgtime(mod(i-1,stride)+1) = toc;
		if(doCompare)
			gmvtest_struct2 = test_struct2;
			clear('test_struct2');
			tic;
			load('res/test_struct2.mat');
			mvgavgtimeload(mod(i-1,strideload)+1) = toc;
			[similarity,gmv,ld] = compstruct(gmvtest_struct2, test_struct2);
			if(~isempty(gmv) || ~isempty(ld))
				%diffs = find(gmv ~= ld);
				%mindiffind = min(find(gmv ~= ld));
				error('getmatvar failed to load correctly on test_struct2')
			end
			clear('gmvtest_struct2');
		end
		
		clear('test_struct2');
		delete('res/test_struct1.mat');
	end
	if(doCompare)
		avgmultiplier = sum(mvgavgtimeload(1:min(strideload,i))./mvgavgtime(1:min(strideload,i)))/min(strideload,i);
		timestr = sprintf('%d/%d | avg multiplier:%5.8f',i,numtests,avgmultiplier);
		fprintf([repmat('\b',1,lents) timestr]);
		lents = numel(timestr);
	else
		timestr = sprintf('%d/%d',i,numtests);
		fprintf([repmat('\b',1,lents) timestr]);
		lents = numel(timestr);
	end
end
fprintf('\n');