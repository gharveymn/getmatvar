addpath('res')
addpath('bin')
%file = 'res/my_struct.mat';
%vars = {'my_struct','my_struct.array', 'my_struct.cell', 'cell', 'string'};
%file = 'C:\workspace\matlab\RonZ\data\optData_ESTrade.mat';
%vars = {'extPar'};
file = 'res/t.mat';
vars = {'t'};
domemory = false;
numtests = 100;
memvals = rand(numtests+1,1);
lents = 0;


if(domemory)
	userview = memory;
	disp(userview.MemUsedMATLAB)
	memvals(1) = userview.MemUsedMATLAB;
	for i = 1:numtests

		getmatvar(file, vars{:});

		timestr = sprintf('%d/%d',i,numtests);
		fprintf([repmat('\b',1,lents) timestr]);
		lents = numel(timestr);
		userview = memory;
		memvals(i+1) = userview.MemUsedMATLAB;
	end
	fprintf('\n');
	userview = memory;
	disp(userview.MemUsedMATLAB)
	plot(memvals);
	xlim([0,numtests]);
else
	userview = memory;
	disp(userview.MemUsedMATLAB)
	for i = 1:numtests

		getmatvar(file, vars{:});

		timestr = sprintf('%d/%d',i,numtests);
		fprintf([repmat('\b',1,lents) timestr]);
		lents = numel(timestr);
	end
	fprintf('\n');
	userview = memory;
	disp(userview.MemUsedMATLAB)
end
clear file vars i numtests userview

