#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace trollfx::esp {

// Deliberately partial ES5510 interpreter. Values are held as signed Q1.23
// words; this is a research core, not a cycle-accurate or product-compatible
// implementation. See docs/HANDOFF_EMULATION.md for implemented semantics.
class ES5510Minimal {
public:
    static constexpr std::size_t kGprCount = 192;
    static constexpr std::size_t kProgramWords = 160;
    static constexpr std::uint32_t kMask24 = 0x00ffffffu;

    void reset() noexcept {
        gpr_.fill(0); program_.fill(0); pc_=0; mac_=0; ccr_=0; halted_=false;
    }
    bool loadProgram(const std::uint64_t* words, std::size_t count) noexcept {
        if (!words || count > kProgramWords) return false;
        program_.fill(0); for (std::size_t i=0;i<count;++i) program_[i]=words[i]&0xffffffffffffull;
        pc_=0; halted_=false; return true;
    }
    void setRegister(std::size_t i, std::int32_t v) noexcept { if(i<kGprCount) gpr_[i]=sign24(v); }
    std::int32_t registerValue(std::size_t i) const noexcept { return i<kGprCount?gpr_[i]:0; }
    std::int64_t accumulator() const noexcept { return mac_; }
    std::size_t pc() const noexcept { return pc_; }
    bool halted() const noexcept { return halted_; }

    // One instruction per call. The instruction field layout follows the
    // publicly described/MAME-disassembled ES5510 word layout.
    void step() noexcept {
        if(halted_) return;
        const std::uint64_t w=program_[pc_];
        const unsigned a=(w>>16)&255u, b=(w>>24)&255u, c=(w>>32)&255u, d=(w>>40)&255u;
        const unsigned op=(w>>12)&15u;
        if(op==15u) { halted_=true; return; }
        const std::int32_t av=read(a), bv=read(b);
        const std::int32_t cv=read(c), dv=read(d);
        const bool accumulate=((w>>6)&1u)!=0;
        const std::int64_t product=static_cast<std::int64_t>(cv)*dv;
        mac_=wrap48(accumulate?mac_+product:product);
        const std::int32_t mr=sign24(static_cast<std::int32_t>(mac_>>23));
        const std::int32_t result=alu(op,av,bv);
        if(op!=4u && a<kGprCount) gpr_[a]=result; // CMP updates flags only
        if(c<kGprCount) gpr_[c]=mr;
        pc_=(pc_+1)%kProgramWords;
    }

    // Minimal host audio bridge: each sample loads stereo Q1.23 into GPR 0/1,
    // executes instructionCount instructions, then reads GPR 2/3 as output.
    void processFrame(float inL,float inR,float& outL,float& outR,unsigned instructionCount) noexcept {
        setRegister(0,floatToQ23(inL)); setRegister(1,floatToQ23(inR));
        for(unsigned i=0;i<instructionCount && !halted_;++i) step();
        outL=q23ToFloat(registerValue(2)); outR=q23ToFloat(registerValue(3));
    }
    static std::uint64_t encode(unsigned a,unsigned b,unsigned c,unsigned d,unsigned op,
                                bool macAccumulate=false) noexcept {
        return (std::uint64_t(d&255)<<40)|(std::uint64_t(c&255)<<32)|
               (std::uint64_t(b&255)<<24)|(std::uint64_t(a&255)<<16)|
               (std::uint64_t(op&15)<<12)|(std::uint64_t(macAccumulate)<<6);
    }
    static std::int32_t sign24(std::int64_t x) noexcept {
        std::uint32_t u=static_cast<std::uint32_t>(x)&kMask24;
        return (u&0x800000u)?static_cast<std::int32_t>(u|0xff000000u):static_cast<std::int32_t>(u);
    }
private:
    static std::int64_t wrap48(std::int64_t x) noexcept {
        constexpr std::uint64_t m=(1ull<<48)-1; std::uint64_t u=static_cast<std::uint64_t>(x)&m;
        return (u&(1ull<<47))?static_cast<std::int64_t>(u|~m):static_cast<std::int64_t>(u);
    }
    std::int32_t read(unsigned i) const noexcept { return i<kGprCount?gpr_[i]:0; }
    static std::int32_t alu(unsigned op,std::int32_t a,std::int32_t b) noexcept {
        switch(op) {
        case 0: return sign24(static_cast<std::int64_t>(a)+b);
        case 1: return sign24(static_cast<std::int64_t>(a)-b);
        case 2: return static_cast<std::int32_t>((static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b))&kMask24);
        case 3: return static_cast<std::int32_t>((static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b))&kMask24);
        case 4: return a-b;
        case 5: return sign24(a&b); case 6:return sign24(a|b); case 7:return sign24(a^b);
        case 8: return a<0?sign24(-static_cast<std::int64_t>(a)):a;
        case 9: return b;
        case 10:return sign24(static_cast<std::int64_t>(b)*4);
        case 11:return sign24(static_cast<std::int64_t>(b)*256);
        case 12:return sign24(static_cast<std::uint32_t>(b)<<15);
        case 13:return sign24(static_cast<std::int64_t>(b)-a);
        case 14:return b>>1;
        default:return 0;
        }
    }
    static std::int32_t floatToQ23(float x) noexcept {
        if (x > 0.99999988f) x = 0.99999988f;
        if (x < -1.0f) x = -1.0f;
        return static_cast<std::int32_t>(x*8388608.0f);
    }
    static float q23ToFloat(std::int32_t x) noexcept { return static_cast<float>(x)/8388608.0f; }
    std::array<std::int32_t,kGprCount> gpr_{};
    std::array<std::uint64_t,kProgramWords> program_{};
    std::size_t pc_=0; std::int64_t mac_=0; std::uint32_t ccr_=0; bool halted_=false;
};
}
