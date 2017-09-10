addpath('res')
addpath('src')
file = 'res/nested.mat';
vars = {'my_struct'};

userview = memory;
disp(userview.MemUsedMATLAB)
tic
getmatvar(file, vars{:});
toc
userview = memory;
disp(userview.MemUsedMATLAB)

tic
%load(file)
toc

clear file vars
