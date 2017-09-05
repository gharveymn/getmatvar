numtests = 10000;
userview = memory;
disp(userview.MemUsedMATLAB)
for i = 1:numtests
	s = getmatvar('res/my_array.mat','my_array');
end
userview = memory;
disp(userview.MemUsedMATLAB)

