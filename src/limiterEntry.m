function [ret] = limiterEntry (fname)
% LIMITERENTRY

dmat = importdata (fname);
nn = size (dmat, 1);
idx = 1;
minPackets = 8;

while (idx < nn)
    eidx = idx + minPackets;
    if (eidx > nn)
        eidx = nn;
    end
    limiterStep (dmat(idx:eidx,:));
    idx = eidx;
end

ret = [];

    

