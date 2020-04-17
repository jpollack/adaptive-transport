function [ctx] = limiterStep(ctx, dmat)
% LIMITERSTEP

% ind = find (dmat(:,2) >= ctx(1).tsUpdated);
% dmat = dmat(ind,:);

ind = find (dmat(:,3));
nrecv = numel(ind);
ndropped = size(dmat, 1) - nrecv;

x0 = dmat(ind(1),2);

yv = dmat(ind,3) - dmat(ind,2);
xv = dmat(ind,2) - x0;

ii = ctx.ii + 1;

ctx.ymin(ii) = min(yv);
ctx.ymax(ii) = max(yv);

if (ii > 3)
    ctx.symin(ii) = mean(ctx.ymin(ii-3:ii));
    ctx.symax(ii) = mean(ctx.ymax(ii-3:ii));
end
sc = 0;
if (ii > 4)
    %    ctx.gsymin(ii) = (ctx.symin(ii) - ctx.symin(ii-1)) / abs(ctx.symin(ii-1));
    %    ctx.gsymax(ii) = (ctx.symax(ii) - ctx.symax(ii-1)) / abs(ctx.symax(ii-1));
    ctx.gsymin(ii) = (ctx.symin(ii) - ctx.symin(ii-1));
    ctx.gsymax(ii) = (ctx.symax(ii) - ctx.symax(ii-1));
    ctx.csymin(ii) = sum(ctx.gsymin(ctx.li:ii));
    sc = ctx.csymin(ii) / abs(ctx.symin(ii-1));
end
ctx.state(ii) = 0;

% delay has increased by more than 10%
if (sc > 0.1)
    ctx.state(ii) = -1;
end

if (sc < -0.05)
    ctx.state(ii) = 1;
end

if (ctx.state(ii))
    ctx.tsUpdated = ctx.tsNow;
    ctx.li = ii;
end

ctx.ii = ii;

