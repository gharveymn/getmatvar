addpath('tests');
addpath('bin');
clear;

rng('shuffle')
numtests = 1;
numsamples = 1000;
lents = 0;

maxDepth = 1;
minelem = 50000;
maxelem = 100000;
maxElementsv = round(linspace(minelem,maxelem,numtests));
ignoreUnusables = true;
stride = numsamples;
mvgavgtime = zeros(stride,1);
mvgavgtimeload = zeros(stride,1);
avgnumelems = zeros(stride,1);
data = zeros(numtests,2);

doplot = false;
donames = false;
doCompare = true;
numelems = 0;
avgmultiplier = 0;

for j = 1:numtests
	
	maxElements = maxElementsv(j);
	for i = 1:numsamples
		if(mod(i,2) == 1)
			[test_struct1, avgnumelems(mod(i-1,stride)+1), names]  = randVarGen(maxDepth, maxElements, ignoreUnusables, donames, 'test_struct1');
			save('res/test_struct1.mat', 'test_struct1');
			clear('test_struct1');
			tic;
			getmatvar('res/test_struct1.mat');
			mvgavgtime(mod(i-1,stride)+1) = toc;
			if(doCompare)
				gmvtest_struct1 = test_struct1;
				clear('test_struct1');
				tic;
				load('res/test_struct1.mat');
				mvgavgtimeload(mod(i-1,stride)+1) = toc;
				
				[similarity,gmv,ld] = compstruct(gmvtest_struct1, test_struct1);
				if(~isempty(gmv) || ~isempty(ld))
					%diffs = find(gmv ~= ld);
					%mindiffind = min(find(gmv ~= ld));
					error('getmatvar failed to load test_struct1 correctly')
				end
				clear('gmvtest_struct1');
			end
			
			if(donames)
				save('res/test_struct1nametest.mat', 'names');
				for p = 1:numel(names)
					gmvret = getmatvar('res/test_struct1.mat',names{p});
					eval(['[similarity,gmv,ld] = compstruct(gmvret, ' names{p} ');']);
					if(~isempty(gmv) || ~isempty(ld))
						%diffs = find(gmv ~= ld);
						%mindiffind = min(find(gmv ~= ld));w
						error(['getmatvar failed to select ' names{p} 'correctly']);
					end
				end
				delete('res/test_struct1nametest.mat');
			end
			clear('test_struct1');
			
			if(i > 2)
				delete('res/test_struct2.mat');
			end

		else
			[test_struct2, avgnumelems(mod(i-1,stride)+1), names] = randVarGen(maxDepth, maxElements, ignoreUnusables, donames, 'test_struct2');
			save('res/test_struct2.mat', 'test_struct2');
			clear('test_struct2');
			tic;
			getmatvar('res/test_struct2.mat');
			mvgavgtime(mod(i-1,stride)+1) = toc;
			if(doCompare)
				gmvtest_struct2 = test_struct2;
				clear('test_struct2');
				tic;
				load('res/test_struct2.mat');
				mvgavgtimeload(mod(i-1,stride)+1) = toc;
				
				[similarity,gmv,ld] = compstruct(gmvtest_struct2, test_struct2);
				if(~isempty(gmv) || ~isempty(ld))
					%diffs = find(gmv ~= ld);
					%mindiffind = min(find(gmv ~= ld));
					error('getmatvar failed to load test_struct2 correctly')
				end
				clear('gmvtest_struct2');
			end
			
			if(donames)
				save('res/test_struct2nametest.mat', 'names');
				for p = 1:numel(names)
					gmvret = getmatvar('res/test_struct2.mat',names{p});
					eval(['[similarity,gmv,ld] = compstruct(gmvret, ' names{p} ');']);
					if(~isempty(gmv) || ~isempty(ld))
						%diffs = find(gmv ~= ld);
						%mindiffind = min(find(gmv ~= ld));w
						error(['getmatvar failed to select ' names{p} 'correctly']);
					end
				end
				delete('res/test_struct2nametest.mat');
			end
			clear('test_struct2');
			
			delete('res/test_struct1.mat');
		end
		if(doCompare)
			avgmultiplier = (sum(mvgavgtimeload(1:min(stride,i))./mvgavgtime(1:min(stride,i))))./min(stride,i);
			numelems = sum(avgnumelems(1:min(stride,i)))./min(stride,i);
			timestr = sprintf('Test %d of %d | Sample %d of %d | avg multiplier:%5.8f | avg numelems:%5.8f',j,numtests,i,numsamples,avgmultiplier,numelems);
			fprintf([repmat('\b',1,lents) timestr]);
			lents = numel(timestr);
		else
			timestr = sprintf('%d/%d',i,numsamples);
			fprintf([repmat('\b',1,lents) timestr]);
			lents = numel(timestr);
		end
	end
	
	
	data(j,1) = numelems;
	data(j,2) = avgmultiplier;
	
end

data = sortrows(data);

if(doplot)
	plot(data(:,1),data(:,2));
end
fprintf('\n');