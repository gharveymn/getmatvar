addpath('res')
addpath('src')
file = 'res/my_struct.mat';
vars = {'my_struct','my_struct.array', 'my_struct.cell', 'cell', 'string'};

numtests = 10000;
userview = memory;
disp(userview.MemUsedMATLAB)
for i = 1:numtests
	getmatvar(file, vars{:});
end
userview = memory;
disp(userview.MemUsedMATLAB)

clear file vars i numtests userview

