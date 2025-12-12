#pragma once

#include "RulesParser.h"

#include <RE/Skyrim.h>
#include <toml++/toml.h>

#define RFSR_CONFIG_PATH R"(Data\SKSE\Plugins\RFSS_Configs\)"

using namespace RE;

namespace Sample {
    class RulesManager {
    private:
        std::vector<std::shared_ptr<Rule>> rules;
    public:
        [[nodiscard]] static const RulesManager& GetSingleton() noexcept;
        [[nodiscard]] BGSFootstepSet* Apply(Actor*, BGSFootstepSet*) const noexcept;
    };
}  // namespace Sample