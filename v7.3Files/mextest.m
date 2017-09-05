addpath('res')
addpath('src')
file = 'res/my_struct.mat';
var = 'my_struct';
tic
s = getmatvar(file,var);
toc

tic
load(file)
toc
