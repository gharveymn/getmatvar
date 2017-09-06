function getmatvar(filename, varargin)
	s = getMatVar(filename, varargin{:});
	vn = fieldnames(s);
	for i = 1:numel(vn)
		assignin('caller', vn{i}, s.(vn{i}));
	end
end