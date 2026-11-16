#include "../experimental/emulation/mame_es5510_standalone.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct Bundle {
    unsigned sampleRate = 44100;
    std::uint8_t hostSerial = 0x20;
    std::vector<std::uint64_t> instructions;
    std::vector<std::pair<std::uint32_t, std::int16_t>> dram;
    std::vector<std::pair<std::uint8_t, std::uint32_t>> gpr;
    // Per-frame host-control schedule; values are raw 24-bit GPR units. For a
    // MEMSIZ shift of zero, one unit addresses one 16-bit delay-RAM word.
    struct LfoGpr {
        std::uint8_t reg;
        double baseUnits, depthUnits, rateHz, phaseCycles;
        double minimumUnits, maximumUnits;
    };
    std::vector<LfoGpr> lfoGpr;
};

bool loadBundle(const std::string& path, Bundle& b, std::string& error) {
    std::ifstream f(path);
    if (!f) { error = "cannot open bundle"; return false; }
    std::string line;
    bool header = false;
    unsigned lineNumber = 0;
    while (std::getline(f,line)) {
        ++lineNumber;
        const auto comment=line.find('#'); if(comment!=std::string::npos) line.resize(comment);
        std::istringstream in(line); std::string key, value;
        if(!(in>>key)) continue;
        if(!header) {
            if(key!="ES5510_BUNDLE" || !(in>>value) || value!="1") { error="line 1 must be ES5510_BUNDLE 1"; return false; }
            header=true; continue;
        }
        if(key=="sample_rate") {
            if(!(in>>b.sampleRate) || b.sampleRate<8000 || b.sampleRate>192000) { error="invalid sample_rate"; return false; }
        } else if(key=="host_serial") {
            unsigned x=0; if(!(in>>std::setbase(0)>>x) || x>255) { error="invalid host_serial"; return false; }
            b.hostSerial=static_cast<std::uint8_t>(x);
        } else if(key=="instruction") {
            std::uint64_t x=0; if(!(in>>std::setbase(0)>>x) || x>0xffffffffffffULL || b.instructions.size()>=trollfx::esp::MameES5510::kInstructionCount) { error="invalid/too many instruction words"; return false; }
            b.instructions.push_back(x);
        } else if(key=="gpr") {
            unsigned reg=0; std::uint32_t x=0; if(!(in>>std::setbase(0)>>reg>>std::setbase(0)>>x) || reg>255 || x>0xffffff) { error="invalid gpr record"; return false; }
            b.gpr.emplace_back(static_cast<std::uint8_t>(reg),x);
        } else if(key=="dram") {
            std::uint32_t address=0; int value16=0; if(!(in>>std::setbase(0)>>address>>value16) || address>=trollfx::esp::MameES5510::kDramWords || value16<-32768 || value16>32767) { error="invalid dram record"; return false; }
            b.dram.emplace_back(address,static_cast<std::int16_t>(value16));
        } else if(key=="lfo_gpr") {
            unsigned reg=0; Bundle::LfoGpr lfo{};
            if(!(in>>reg>>lfo.baseUnits>>lfo.depthUnits>>lfo.rateHz>>lfo.phaseCycles>>lfo.minimumUnits>>lfo.maximumUnits) ||
               reg>=192 || !std::isfinite(lfo.baseUnits) || !std::isfinite(lfo.depthUnits) ||
               !std::isfinite(lfo.rateHz) || !std::isfinite(lfo.phaseCycles) ||
               !std::isfinite(lfo.minimumUnits) || !std::isfinite(lfo.maximumUnits) ||
               lfo.baseUnits<0 || lfo.depthUnits<0 || lfo.rateHz<0 ||
               lfo.minimumUnits<0 || lfo.maximumUnits<lfo.minimumUnits ||
               lfo.maximumUnits>0xffffff) { error="invalid lfo_gpr record"; return false; }
            lfo.reg=static_cast<std::uint8_t>(reg);
            b.lfoGpr.push_back(lfo);
        } else { error="unknown record at line "+std::to_string(lineNumber)+": "+key; return false; }
        std::string extra; if(in>>extra) { error="unexpected trailing data on line "+std::to_string(lineNumber); return false; }
    }
    if(!header || b.instructions.empty()) { error="bundle must contain a header and at least one instruction"; return false; }
    bool end=false; for(auto w:b.instructions) if(((w>>12)&15u)==15u) end=true;
    if(!end) { error="program does not contain an END microinstruction"; return false; }
    return true;
}

void put16(std::ofstream& f,std::uint16_t v){f.put(char(v&255));f.put(char(v>>8));}
void put32(std::ofstream& f,std::uint32_t v){put16(f,std::uint16_t(v));put16(f,std::uint16_t(v>>16));}
std::uint16_t get16(const unsigned char* p){return std::uint16_t(p[0])|std::uint16_t(p[1])<<8;}
std::uint32_t get32(const unsigned char* p){return std::uint32_t(get16(p))|std::uint32_t(get16(p+2))<<16;}

bool readWav(const std::string& path,unsigned& sr,std::vector<std::int16_t>& pcm,std::string& error){
    std::ifstream f(path,std::ios::binary); if(!f){error="cannot open input WAV";return false;}
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),{});
    if(bytes.size()<44||std::string(reinterpret_cast<char*>(bytes.data()),4)!="RIFF"||std::string(reinterpret_cast<char*>(bytes.data()+8),4)!="WAVE"){error="invalid RIFF/WAVE file";return false;}
    std::uint16_t format=0,channels=0,bits=0;std::uint32_t dataSize=0;bool gotFmt=false,gotData=false;
    for(std::size_t p=12;p+8<=bytes.size();){const std::uint32_t n=get32(bytes.data()+p+4);if(n>bytes.size()-p-8){error="truncated WAV chunk";return false;}
        if(std::string(reinterpret_cast<char*>(bytes.data()+p),4)=="fmt "&&n>=16){const auto*q=bytes.data()+p+8;format=get16(q);channels=get16(q+2);sr=get32(q+4);bits=get16(q+14);gotFmt=true;}
        if(std::string(reinterpret_cast<char*>(bytes.data()+p),4)=="data"){dataSize=n;const auto*q=bytes.data()+p+8;if(gotFmt&&format==1&&channels==2&&bits==16){pcm.resize(dataSize/2);for(std::size_t i=0;i<pcm.size();++i)pcm[i]=static_cast<std::int16_t>(get16(q+2*i));gotData=true;break;}}
        p+=8+n+(n&1u);
    }
    if(!gotFmt||!gotData||format!=1||channels!=2||bits!=16||sr<8000||sr>192000||dataSize%4){error="only stereo 16-bit PCM WAV is supported";return false;}
    return true;
}

bool writeWav(const std::string& path,unsigned sr,const std::vector<std::int16_t>& pcm){
    if(pcm.size()%2) return false;
    std::ofstream f(path,std::ios::binary);
    if(!f) return false;
    const auto bytes=static_cast<std::uint32_t>(pcm.size()*2);
    f.write("RIFF",4);put32(f,36+bytes);f.write("WAVEfmt ",8);put32(f,16);put16(f,1);put16(f,2);put32(f,sr);put32(f,sr*4);put16(f,4);put16(f,16);f.write("data",4);put32(f,bytes);
    for(auto x:pcm) put16(f,static_cast<std::uint16_t>(x));
    return f.good();
}

bool run(const std::string& bundlePath,const std::string& inPath,const std::string& outPath){
    Bundle b;std::string error;if(!loadBundle(bundlePath,b,error)){std::cerr<<"bundle error: "<<error<<'\n';return false;}
    unsigned sampleRate=0;std::vector<std::int16_t> in;if(!readWav(inPath,sampleRate,in,error)){std::cerr<<"input error: "<<error<<'\n';return false;}
    if(sampleRate!=b.sampleRate){std::cerr<<"sample-rate mismatch: bundle="<<b.sampleRate<<" WAV="<<sampleRate<<'\n';return false;}
    trollfx::esp::MameES5510 cpu;if(!cpu.prepare()||!cpu.loadProgram(b.instructions.data(),b.instructions.size())){std::cerr<<"engine/program initialization failed\n";return false;}
    cpu.setHostSerial(b.hostSerial);for(auto [reg,value]:b.gpr)cpu.setRegister(reg,value);for(auto [addr,value]:b.dram)cpu.setDramWord(addr,value);
    std::vector<std::int16_t> out(in.size());std::uint64_t peak=0;
    constexpr double twoPi=6.283185307179586476925286766559;
    for(std::size_t i=0;i<in.size()/2;++i){
        for(const auto& lfo:b.lfoGpr){
            const double phase=twoPi*(lfo.rateHz*static_cast<double>(i)/sampleRate+lfo.phaseCycles);
            const double value=std::clamp(lfo.baseUnits+lfo.depthUnits*std::sin(phase),lfo.minimumUnits,lfo.maximumUnits);
            const auto offset=static_cast<std::uint32_t>(std::llround(value));
            cpu.setRegister(lfo.reg,offset);
        }
        std::array<std::int16_t,6> inputs{};inputs[4]=in[2*i];inputs[5]=in[2*i+1];std::array<std::int16_t,2> outputs{};cpu.processFrame(inputs,outputs);out[2*i]=outputs[0];out[2*i+1]=outputs[1];peak=std::max<std::uint64_t>(peak,std::max(std::abs(int(outputs[0])),std::abs(int(outputs[1]))));}
    if(!writeWav(outPath,sampleRate,out)){std::cerr<<"failed writing output WAV\n";return false;}
    std::cout<<"Rendered "<<cpu.frames()<<" frames; ";
    const auto ipf=cpu.frames()?cpu.instructionsExecuted()/cpu.frames():0;
    std::cout<<ipf<<" microinstructions/frame, "<<cpu.instructionsExecuted()<<" total, peak="<<peak<<"\n";
    return true;
}

bool makeImpulse(const std::string& path,unsigned sr,unsigned seconds){
    std::vector<std::int16_t> pcm(static_cast<std::size_t>(sr)*seconds*2,0);if(!pcm.empty()){pcm[0]=24576;pcm[1]=8192;}return writeWav(path,sr,pcm);
}
}

int main(int argc,char**argv){
    if(argc==2&&std::string(argv[1])=="--help"){std::cout<<"Usage: es5510-render BUNDLE.esp INPUT.wav OUTPUT.wav\n       es5510-render --make-impulse OUTPUT.wav [44100] [4]\n";return 0;}
    if(argc>=3&&std::string(argv[1])=="--make-impulse") {const unsigned sr=argc>3?static_cast<unsigned>(std::stoul(argv[3])):44100;const unsigned sec=argc>4?static_cast<unsigned>(std::stoul(argv[4])):4;return makeImpulse(argv[2],sr,sec)?0:1;}
    if(argc!=4){std::cerr<<"Use --help for usage\n";return 2;}
    return run(argv[1],argv[2],argv[3])?0:1;
}
