function getmatvar(filename, varargin)
	
% 	FUNCTION_HANDLE_SIG = 'R2VuZSBIYXJ2ZXkgOik=';
% 	FUNCTION_HANDLE_SIG_LEN = 20;
	
	[Q291cnRuZXkgQm9ubmVyIDop, R2VuZSBIYXJ2ZXkgOik] = getmatvar_(filename, varargin{:});
	eval(R2VuZSBIYXJ2ZXkgOik);
	vn = fieldnames(Q291cnRuZXkgQm9ubmVyIDop);
	for i = 1:numel(vn)
% 		if(ischar(Q291cnRuZXkgQm9ubmVyIDop.(vn{i})))
% 			%check for the function handle signature
% 			if(strncmp(Q291cnRuZXkgQm9ubmVyIDop.(vn{i}), FUNCTION_HANDLE_SIG, FUNCTION_HANDLE_SIG_LEN))
% 				eval(['Q291cnRuZXkgQm9ubmVyIDop.(vn{i}) = ' Q291cnRuZXkgQm9ubmVyIDop.(vn{i})(21:end) ';']);
% 			end
% 		end
		assignin('caller', vn{i}, Q291cnRuZXkgQm9ubmVyIDop.(vn{i}));
	end
end