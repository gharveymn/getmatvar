function [ret] = randVarGen(maxDepth, maxElements, ignoreUnusables)
	ret = randVarGen_(maxDepth, 1, maxElements, ignoreUnusables);
end


function [ret] = randVarGen_(maxDepth, currDepth, maxElements, ignoreUnusables)
	
	%TODO add sparses
	
	%  Variable Type Key
	% 	1	mxLOGICAL_CLASS,
	% 	2	mxCHAR_CLASS,
	% 	3	mxVOID_CLASS,
	% 	4	mxDOUBLE_CLASS,
	% 	5	mxSINGLE_CLASS,
	% 	6	mxINT8_CLASS,
	% 	7	mxUINT8_CLASS,
	% 	8	mxINT16_CLASS,
	% 	9	mxUINT16_CLASS,
	% 	10	mxINT32_CLASS,
	% 	11	mxUINT32_CLASS,
	% 	12	mxINT64_CLASS,
	% 	13	mxUINT64_CLASS,
	% 	14	mxFUNCTION_CLASS,
	% 	15	mxOPAQUE_CLASS/SPARSES,
	% 	16	mxOBJECT_CLASS,
	% 	17	mxCELL_CLASS,
	% 	18	mxSTRUCT_CLASS
	
	if(maxDepth <= currDepth)
		%dont make another layer
		vartypegen = randi(16);
	else
		vartypegen = randi(18);
	end
	
	if(vartypegen < 17)
		thisMaxElements = maxElements;
	else
		thisMaxElements = 16;
	end
	
	numvarsz = randi(thisMaxElements);
	nums = 1:numvarsz;
	divs = nums(mod(numvarsz, nums) == 0);
	temp = numvarsz;
	dims = {};
	while(true)
		randdiv = divs(randi(numel(divs)));
		dims = [dims {randdiv}];
		temp = temp / randdiv;
		divs = divs(mod(temp,divs) == 0);
		if(temp == 1)
			break;
		end
	end
	
	if(numel(dims) == 1)
		dims = [dims {1}];
	end
	
	switch(vartypegen)
		case(1)
			% 	1	mxLOGICAL_CLASS,
			ret = logical(rand(dims{:}) > 0.5);
		case(2)
			% 	2	mxCHAR_CLASS,
			ret = char(randi(65536,dims{:})-1);
		case(3)
			% 	3	mxVOID_CLASS/SPARSE,
			%reserved, make a double instead or sparse
			if(numel(dims) == 2)
				ret = sparse(logical(rand(dims{:}) > 0.5));
				if(randi(2) == 1)
					ret = ret.*rand(dims{:},'double');
				end
			else
				ret = rand(dims{:},'double');
			end
		case(4)
			% 	4	mxDOUBLE_CLASS,
			ret = rand(dims{:},'double');
		case(5)
			% 	5	mxSINGLE_CLASS,
			ret = rand(dims{:},'single');
		case(6)
			% 	6	mxINT8_CLASS,
			ret = int8(randi(intmax('int8'),dims{:}));
		case(7)
			% 	7	mxUINT8_CLASS,
			ret = uint8(randi(intmax('uint8'),dims{:}));
		case(8)
			% 	8	mxINT16_CLASS,
			ret = int16(randi(intmax('int16'),dims{:}));
		case(9)
			% 	9	mxUINT16_CLASS,
			ret = uint16(randi(intmax('uint16'),dims{:}));
		case(10)
			% 	10	mxINT32_CLASS,
			ret = int32(randi(intmax('int32'),dims{:}));
		case(11)
			% 	11	mxUINT32_CLASS,
			ret = uint32(randi(intmax('uint32'),dims{:}));
		case(12)
			% 	12	mxINT64_CLASS,
			ret = int64(randi(intmax,dims{:}));
		case(13)
			% 	13	mxUINT64_CLASS,
			ret = uint64(randi(intmax,dims{:}));
		case(14)
			% 	14	mxFUNCTION_CLASS,
			if(ignoreUnusables)
				ret = rand(dims{:},'double');
			else
				af1 = @(x) x + 17;
				af2 = @(x,y,z) x*y*z;
				sf1 = @simplefunction1;
				sf2 = @simplefunction2;

				funcs = {af1,af2,sf1,sf2};
				ret = funcs{randi(numel(funcs))};
			end
		case(15)
			% 	15	mxOPAQUE_CLASS,
			% not sure how to generate, generate a sparse array instead
			if(numel(dims) == 2)
				ret = sparse(logical(rand(dims{:}) > 0.5).*rand(dims{:},'double'));
			else
				ret = rand(dims{:},'double');
			end
		case(16)
			% 	16	mxOBJECT_CLASS,
			if(ignoreUnusables)
				ret = rand(dims{:},'double');
			else
				ret = BasicClass(randi(intmax),dims);
			end
		case(17)
			% 	17	mxCELL_CLASS,
			ret = cell(dims{:});
			for k = 1:numel(ret)
				ret{k} = randVarGen_(maxDepth, currDepth + 1, maxElements, ignoreUnusables);
			end
		case(18)
			% 	18	mxSTRUCT_CLASS
			numPossibleFields = 5;
			possibleFields = {'cat',[],'dog',[],'fish',[],'cow',[],'raven',[]};
			ret(dims{:}) = struct(possibleFields{1:2*randi(numPossibleFields)});
			retFields = fieldnames(ret);
			for k = 1:numel(ret)
				for j = 1:numel(retFields)
					eval(['ret(k).' retFields{j} ' = randVarGen_(maxDepth, currDepth + 1, maxElements, ignoreUnusables);']);
				end
			end
			
	end
	
end

