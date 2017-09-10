addpath('res')
addpath('src')
file = 'C:\workspace\matlab\RonZ\data\optData_ESTrade.mat';
vars = {'extPar'};
userview = memory;
disp(userview.MemUsedMATLAB)
tic
getmatvar(file, vars{:});
toc
userview = memory;
disp(userview.MemUsedMATLAB)

tic
load(file)
toc

clear file vars numtests userview