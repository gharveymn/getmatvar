numtests = 10000;
m = zeros(1,numtests);
for i = 1:numtests
	userview = memory;
	s = getmatvar('my_struct.mat','my_struct');
	m(i) = userview.MemUsedMATLAB;
end
plot(m);

