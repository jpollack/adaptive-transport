% function [ret] = limiterEntry (fname)
% LIMITERENTRY

clear all;
close all;

fname = '/home/joshua/metadata.tsv';

fid = fopen (fname);
C = textscan(fid, '%u64\t%u64\t%u64', 'CollectOutput', 1);
fclose (fid);

dmat = C{1};

% dmat = importdata (fname);
nn = size (dmat, 1);
idx = 1;
minPackets = 8;

ctx = struct([]);
ctx(1).tsUpdated = 0;
ctx.tsNow = dmat(1,2);
ctx.ii = 0;
ctx.ymin = [];
ctx.ymax = [];
ctx.symin = [];
ctx.symax = [];
ctx.gsymin = [];
ctx.gsymax = [];
ctx.csymin = [];
ctx.state = [];
ctx.action = [];
ctx.li = 1;
bw = 2000000;
while ((idx < nn) && (ctx.tsNow < (dmat(end,2) + 1000000)))
    
    aind = find ( (0 < dmat(idx:end,3)) & (dmat(idx:end,3) < ctx.tsNow) & (dmat(idx:end,2) >= ctx.tsUpdated));

    if (numel(aind) >= minPackets)
        xv = idx:(idx+(max(aind) - 1));
        ctx = limiterStep (ctx, dmat(xv,:));
        idx = max(xv) + 1;
    end
    ctx.tsNow = ctx.tsNow + 5000 + round(abs(2000 * randn(1)));
end


    

