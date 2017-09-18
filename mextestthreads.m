addpath('res')
addpath('bin')
file = 'res/my_struct1.mat';
vars = {''};
numtests = 100;
numsubtests = 5;
maxthreads = 7;
avgtimes = zeros(7,1);
maxdims = 32;
lo = 1e4;
hi = 1e8;
besttimes = zeros(numtests,1);
realnumelems = zeros(numtests,1);
numdims = zeros(numtests,1);
lents = 0;


userview = memory;
disp(userview.MemUsedMATLAB)
for j = 1:numtests
	rne = 0;
	while(rne < lo || rne > hi)
		numdims(j) = randi(maxdims-1)+1;
		numelems = randi(hi-lo) + lo;
		avg = floor(nthroot(numelems,numdims(j)));
		dims = zeros(1,numdims(j));
		for j = 1:numdims(j)
			dims(j) = max(avg + randi(6)-3,1);
		end
		rne = prod(dims);
	end
	realnumelems(j) = rne;
	t = rand(dims);
	save('res/t.mat','t');
	
	for j = 1:maxthreads
		avgtimes(j) = 0;
		for k = 1:numsubtests
			tic
			getmatvar('res/r.mat','-threads',j);
			avgtimes(j) = avgtimes(j) + toc/numsubtests;
		end
	end
	[~,besttimes(j)] = min(avgtimes);
	
	timestr = sprintf('%d/%d',j,numtests);
	fprintf([repmat('\b',1,lents) timestr]);
	lents = numel(timestr);
end
fprintf('\n');
clear r
userview = memory;
disp(userview.MemUsedMATLAB)

figure(1)
hold on
plot(realnumelems,besttimes);
hold off

figure(2)
hold on
scatter3(realnumelems,numdims,besttimes, [], [besttimes/maxthreads, zeros(numtests,1), 1-besttimes/maxthreads], '.');
hold off

%tic
%getmatvar(file,'-threads',3);
%toc
