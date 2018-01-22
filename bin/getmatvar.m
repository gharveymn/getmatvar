function [varargout] = getmatvar(filename, varargin)
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