function getmatvar(filename, varargin)
	Q291cnRuZXkgQm9ubmVyIDop = getmatvar_(filename, varargin{:});
	vn = fieldnames(Q291cnRuZXkgQm9ubmVyIDop);
	for i = 1:numel(vn)
		if(ischar(Q291cnRuZXkgQm9ubmVyIDop.(vn{i})))
			if(strncmp(Q291cnRuZXkgQm9ubmVyIDop.(vn{i}), 'R2VuZSBIYXJ2ZXkgOik=', 20))
				eval(['Q291cnRuZXkgQm9ubmVyIDop.(vn{i}) = ' Q291cnRuZXkgQm9ubmVyIDop.(vn{i})(21:end) ';']);
			end
		end
		assignin('caller', vn{i}, Q291cnRuZXkgQm9ubmVyIDop.(vn{i}));
	end
end