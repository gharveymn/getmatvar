disp(getmemstr)

addpath('res')
addpath('bin')
%file = 'res/my_struct.mat';
%vars = {'my_struct','my_struct.array', 'my_struct.cell', 'cell', 'string'};
%file = 'C:\workspace\matlab\RonZ\data\optData_ESTrade.mat';
%vars = {'extPar'};
file = 'res/my_struct1.mat';
vars = {'my_struct'};
domemory = false;
compare = false;
numtests = 10000;
stride = 100;
mvgavgtime = zeros(stride,1);
strideload = 100;
mvgavgtimeload = zeros(strideload,1);
memvals = rand(numtests+1,1);
lents = 0;
if(domemory)
	userview = memory;
	memvals(1) = userview.MemUsedMATLAB;
	for i = 1:numtests

		getmatvar(file, vars{:}, '-sw');

		timestr = sprintf('%d/%d',i,numtests);
		fprintf([repmat('\b',1,lents) timestr]);
		lents = numel(timestr);
		userview = memory;
		memvals(i+1) = userview.MemUsedMATLAB;
	end
	fprintf('\n');
	plot(memvals);
	xlim([0,numtests]);
else
	for i = 1:numtests
		tic;
		getmatvar(file, vars{:}, '-suppress-warnings');
		mvgavgtime(mod(i,stride+1)+1) = toc;
		if(compare)
			tic;
			load(file);
			mvgavgtimeload(mod(i,strideload+1)+1) = toc;
			
			timestr = sprintf('%d/%d | avg:%5.8f vs %5.8f',i,numtests,sum(mvgavgtime)/min(stride,i),sum(mvgavgtimeload)/min(strideload,i));
			fprintf([repmat('\b',1,lents) timestr]);
			lents = numel(timestr);
			
		else
			timestr = sprintf('%d/%d | avg:%5.8f',i,numtests,sum(mvgavgtime)/min(stride,i));
			fprintf([repmat('\b',1,lents) timestr]);
			lents = numel(timestr);
		end
	end
	fprintf('\n');
end
disp(getmemstr)
