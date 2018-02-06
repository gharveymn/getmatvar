function [varargout] = getmatvar(filename, varargin)
	%GETMATVAR a fast partial loader for 7.3+ MAT-files
	% Load a variable located in a MAT-file by specifying
	% the file and variable name, or load the entire MAT-file
	% by only specifying the filename.
	% 
	% Usage:
	%	getmatvar(filename) loads an entire MAT-file.
	%	var = getmatvar(filename,'var') loads a variable from a MAT-file.
	%	[var1,...,varN] = getmatvar(filename,'var1',...,'varN') loads multiple variables from a MAT-file.
	% 
	%	getmatvar(__,'-t',n) specifies the number of threads to use 
	%	in the next argument n.
	%
	%	getmatvar(__,'-st') restricts getmatvar to a single thread.
	%
	%	getmatvar(__,'-sw') suppresses warnings from getmatvar.
	% 
	% Examples:
	%	getmatvar('my_workspace.mat') % All variables
	%	my_struct = getmatvar('my_workspace.mat', 'my_struct') % Only variable my_struct
	
	[Q291cnRuZXkgQm9ubmVyIDop] = getmatvar_(filename, varargin{:});
	%eval(R2VuZSBIYXJ2ZXkgOik);
	vn = fieldnames(Q291cnRuZXkgQm9ubmVyIDop);
	if(nargin == 1 || varargin{1}(1) == '-')
		for i = 1:numel(vn)
			assignin('caller', vn{i}, Q291cnRuZXkgQm9ubmVyIDop.(vn{i}));
		end
	else
		for i = 1:max(nargout,1)
			varargout{i} = Q291cnRuZXkgQm9ubmVyIDop.(vn{i});
		end
	end
end