addpath('res')
addpath('src')
file = 'C:\workspace\matlab\RonZ\data\optData_ESTrade.mat';
var = 'extPar';
tic
s = getmatvar(file, var);
toc

tic
load(file)
toc
