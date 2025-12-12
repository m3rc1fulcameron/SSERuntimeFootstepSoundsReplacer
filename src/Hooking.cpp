#include "Hooking.h"
#include "RulesManager.h"

using namespace Sample;
using namespace RE;
using namespace REL;
using namespace SKSE;

namespace {
    static const RulesManager* rulesManager;
    static TESDataHandler* dataHandler;

    const BGSFootstep* HookedGetFootstep(BGSFootstepSet*, uint32_t, BSFixedString&);
    Relocation<decltype(HookedGetFootstep)>& GetHookedGetFootstep() noexcept {
        static Relocation<decltype(HookedGetFootstep)> value(RELOCATION_ID(0, 36209), 0x473);
        return value;
    }
    Relocation<decltype(HookedGetFootstep)> OriginalGetFootstep;

    const BGSFootstep* HookedGetFootstep(BGSFootstepSet* fss_ptr, uint32_t unk, BSFixedString &tag) {
        log::trace("Intercepted call to GetFootstep");
        log::trace("tag_ptr: {}; tag: {}", fmt::ptr(&tag), tag);

        if (!dataHandler) {
            log::critical("Data handler not yet initialized (how is this being called?)");
            return OriginalGetFootstep(fss_ptr, unk, tag);
        }

        Actor* actor_mbbe = ((ActorHandle*)((char*)&tag - 0x08))->get().get();

        auto new_fss = rulesManager->Apply(actor_mbbe, fss_ptr);

        return OriginalGetFootstep(new_fss, unk, tag);
    }
}

void Sample::InitializeHook(Trampoline& trampoline) {
    rulesManager = &(RulesManager::GetSingleton());
    dataHandler = TESDataHandler::GetSingleton();
    OriginalGetFootstep = 
        trampoline.write_call<5>(GetHookedGetFootstep().address(), reinterpret_cast<uintptr_t>(HookedGetFootstep));
    log::info("hook written.");
}
