addpath('res')
addpath('src')
file = 'res/my_struct.mat';
vars = {'my_struct','dbl', 'cell'};

tic
getmatvar(file, vars{:});
toc

tic
%load(file)
toc

clear file vars
