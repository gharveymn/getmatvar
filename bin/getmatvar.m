function getmatvar(filename, varargin)
	[Q291cnRuZXkgQm9ubmVyIDop, R2VuZSBIYXJ2ZXkgOik] = getmatvar_(filename, varargin{:});
	eval(R2VuZSBIYXJ2ZXkgOik);
	vn = fieldnames(Q291cnRuZXkgQm9ubmVyIDop);
	for i = 1:numel(vn)
		assignin('caller', vn{i}, Q291cnRuZXkgQm9ubmVyIDop.(vn{i}));
	end
end