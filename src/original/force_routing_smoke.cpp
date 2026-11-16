#include "force_routing.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main() {
    using namespace trollfx;
    float fbL=routeFeedbackInput(1.0f,.5f,.4f);
    float fbR=routeFeedbackInput(-1.0f,.25f,.4f);
    assert(std::fabs(fbL-1.2f)<1e-6f && std::fabs(fbR+0.9f)<1e-6f);
    float l=0,r=0;
    routeFeedbackOutput(0,.2f,.4f,.8f,.6f,l,r);
    assert(l==.8f&&r==.6f);
    routeFeedbackOutput(1,.2f,.4f,.8f,.6f,l,r);
    assert(std::fabs(l-.5f)<1e-6f&&std::fabs(r-.5f)<1e-6f);
    routeFeedbackOutput(0,1,1,1,1,l,r);
    assert(l==1&&r==1); // bypass/dry pass-through
    std::puts("feedback route smoke PASS: bounded send, FB1/FB2 dry paths, bypass");
}
