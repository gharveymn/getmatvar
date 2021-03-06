addpath('res')
addpath('bin')
file = 'res/my_struct1.mat';
vars = {'my_struct'};

%disp(getmem)
tic
getmatvar(file, vars{:}, '-sw');
%getmatvar(file, '-threads', 0);
toc
%disp(getmem)
%a = t;
%ex = extPar;

% using load function
tic
load(file, vars{:});
toc

%disp(sum(a(:) ~= t(:)))

% if(~isempty(find(my_struct.array ~= 1)))
% 	disp('my_struct1.mat is incorrect')
% else
% 	disp('my_struct1.mat is correct')
% end

%[a,b,c] = compstruct(ex, extPar);

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
clear file vars userview
