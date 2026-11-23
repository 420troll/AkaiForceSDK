#include "../../../framework/mpc_vst2_abi.h"
#include <cmath>
#include <cstdio>
#include <dlfcn.h>
using Entry = AEffect* (*)(audioMasterCallback);
static VstIntPtr host(AEffect*, VstInt32 op, VstInt32, VstIntPtr, void*, float) {
    return op == audioMasterVersion ? 2400 : 0;
}
static AEffect* load(const char* path, void** handle) {
    *handle = dlopen(path, RTLD_NOW);
    if (!*handle) { std::fprintf(stderr, "%s\n", dlerror()); return nullptr; }
    auto entry = reinterpret_cast<Entry>(dlsym(*handle, "VSTPluginMain"));
    if (!entry) return nullptr;
    auto* effect = entry(host);
    if (effect) effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
    return effect;
}
static void close(AEffect* e, void* h) {
    if (e) e->dispatcher(e, effClose, 0, 0, nullptr, 0.0f);
    if (h) dlclose(h);
}
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    void *oldHandle = nullptr, *newHandle = nullptr;
    AEffect* oldFx = load(argv[1], &oldHandle);
    AEffect* newFx = load(argv[2], &newHandle);
    if (!oldFx || !newFx || oldFx->numParams != 7 || newFx->numParams != 13) return 3;
    const float legacy[] = {1.0f, 0.45f, 0.10f, 0.75f, 0.30f, 0.95f, 1.0f};
    for (int i = 0; i < 7; ++i) oldFx->setParameter(oldFx, i, legacy[i]);
    void* chunk = nullptr;
    const VstIntPtr bytes = oldFx->dispatcher(oldFx, effGetChunk, 0, 0, &chunk, 0.0f);
    if (bytes <= 0 || !chunk || !newFx->dispatcher(newFx, effSetChunk, 0, bytes, chunk, 0.0f)) return 4;
    for (int i = 0; i < 7; ++i)
        if (std::fabs(newFx->getParameter(newFx, i) - legacy[i]) > 1e-6f) return 5;
    const float appendedDefaults[] = {0.0f, 0.5f, 0.4f, 0.5f, 1.0f, 0.0f};
    for (int i = 0; i < 6; ++i)
        if (std::fabs(newFx->getParameter(newFx, 7 + i) - appendedDefaults[i]) > 1e-5f) return 6;
    std::puts("V0.1 -> V0.2 state migration: seven existing values preserved; six appended controls defaulted PASS");
    close(oldFx, oldHandle); close(newFx, newHandle);
    return 0;
}
