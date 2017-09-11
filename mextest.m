addpath('res')
addpath('bin')
file = 'my_array.mat';
vars = {'my_array'};

%userview = memory;
%disp(userview.MemUsedMATLAB)
tic
getmatvar(file, vars{:});
toc
%userview = memory;
%disp(userview.MemUsedMATLAB)

% using load function
tic
%load(file);
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
% clear file vars
