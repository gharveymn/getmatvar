addpath('tests');
addpath('bin');
clear;

rng('shuffle')
numtests = 10000;
lents = 0;
for i = 1:numtests
	if(mod(i,2) == 1)
		test_struct1 = randVarGen(1,1,500000);
		save('res/test_struct1.mat', 'test_struct1');
		clear('test_struct1');
		getmatvar('res/test_struct1.mat','-sw');
		clear('test_struct1');
		if(i > 2)
			delete('res/test_struct2.mat');
		end
		
	else
		test_struct2 = randVarGen(3,1,5);
		save('res/test_struct2.mat', 'test_struct2');
		clear('test_struct2');
		getmatvar('res/test_struct2.mat','-sw');
		clear('test_struct2');
		delete('res/test_struct1.mat');
	end
	timestr = sprintf('%d/%d',i,numtests);
	fprintf([repmat('\b',1,lents) timestr]);
	lents = numel(timestr);
end
fprintf('\n');