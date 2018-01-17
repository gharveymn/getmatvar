addpath('tests');
addpath('bin');
clear;

numtests = 1000;
lents = 0;
for i = 1:numtests
	test_struct = randVarGen(2,1,50);
	save('res/test_struct.mat', 'test_struct');
	clear('test_struct');
	getmatvar('res/test_struct.mat');
	clear('test_struct');
	delete('res/test_struct.mat');
	timestr = sprintf('%d/%d',i,numtests);
	fprintf([repmat('\b',1,lents) timestr]);
	lents = numel(timestr);
end