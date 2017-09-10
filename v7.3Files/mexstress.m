addpath('res')
addpath('src')
%file = 'res/my_struct.mat';
%vars = {'my_struct','my_struct.array', 'my_struct.cell', 'cell', 'string'};
file = 'C:\workspace\matlab\RonZ\data\optData_ESTrade.mat';
vars = {'extPar'};

numtests = 100;
lents = 0;
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

clear file vars i numtests userview

