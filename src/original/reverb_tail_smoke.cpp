#include "dp4_reverb.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

static double segmentEnergy(const std::vector<float>& x, int sr, float a, float b) {
    double sum = 0.0;
    for (int i = int(a * sr); i < int(b * sr); ++i) sum += double(x[std::size_t(i)]) * x[std::size_t(i)];
    return sum;
}

static bool render(trollfx::ReverbKind kind, float decay, std::vector<float>& out) {
    constexpr int sr = 44100;
    trollfx::DP4InspiredReverb rv;
    if (!rv.prepare(float(sr))) return false;
    rv.setKind(kind);
    trollfx::ReverbParams p; p.decaySeconds=decay; p.preDelayMs=20.0f; p.mix=1.0f;
    p.diffusion=.7f; p.damping=.35f; p.outputDb=0.0f;
    rv.setParams(p);
    constexpr std::size_t n=sr*2;
    std::vector<float> l(n,0),r(n,0); out.assign(n,0); std::vector<float> orr(n,0);
    l[0]=r[0]=1.0f;
    rv.process(l.data(),r.data(),out.data(),orr.data(),n);
    for(float x:out) if(!std::isfinite(x)) return false;
    return true;
}

int main() {
    for(auto kind:{trollfx::ReverbKind::Room,trollfx::ReverbKind::Hall,trollfx::ReverbKind::Plate,trollfx::ReverbKind::Modulated}) {
        std::vector<float> x;
        if(!render(kind,5.0f,x)) return 1;
        const double e01=segmentEnergy(x,44100,.1f,.2f);
        const double e05=segmentEnergy(x,44100,.5f,.6f);
        const double e1=segmentEnergy(x,44100,1.0f,1.1f);
        if(!(e01>1e-10 && e05>1e-14 && e1>1e-16)) { std::fprintf(stderr,"tail energies %.3e %.3e %.3e\n",e01,e05,e1); return 2; }
    }
    std::vector<float> shortTail,longTail;
    if(!render(trollfx::ReverbKind::Hall,.5f,shortTail)||!render(trollfx::ReverbKind::Hall,8.0f,longTail)) return 3;
    if(!(segmentEnergy(longTail,44100,1.0f,1.1f)>segmentEnergy(shortTail,44100,1.0f,1.1f))) return 4;
    std::puts("reverb_tail_smoke: PASS (four algorithms audible at 0.1/0.5/1 s; decay control changes tail)");
}
