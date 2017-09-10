addpath('res')
addpath('src')
file = 'C:\workspace\matlab\RonZ\data\optData_ESTrade.mat';
var = 'extPar';
userview = memory;
disp(userview.MemUsedMATLAB)
tic
getmatvar(file, var);
toc
userview = memory;
disp(userview.MemUsedMATLAB)

tic
%load(file)
toc

clear file vars numtests userview