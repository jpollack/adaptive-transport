function [ret] = limiterStep(dmat)
% LIMITERSTEP
    
ind = find (dmat(:,3));
nrecv = numel(ind);
ndropped = size(dmat, 1) - nrecv;

x0 = dmat(ind(1),2);

yv = dmat(ind,3) - dmat(ind,2);
xv = dmat(ind,2) - x0;

disp(yv');

ret = yv;
