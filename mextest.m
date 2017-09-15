addpath('res')
addpath('bin')
file = 'res/t.mat';
vars = {'t'};
%file = 'C:\workspace\matlab\RonZ\data\optData_ESTrade.mat';
%vars = {'extPar'};

%userview = memory;
%disp(userview.MemUsedMATLAB)
tic
getmatvar(file,vars{:});
toc
%userview = memory;
%disp(userview.MemUsedMATLAB)

% using load function
tic
load(file);
toc



% using matlab's partial loader
% tic
% mf = matfile(file);
% for i = 1:numel(vars)
% 	eval([vars{i} ' = mf.' vars{i} ';']);
% end
% clear mf
% toc
% clear i
% 
clear file vars
