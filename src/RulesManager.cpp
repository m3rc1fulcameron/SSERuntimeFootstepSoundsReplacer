#include "RulesManager.h"

#include <toml++/toml.h>

#include <filesystem>

#include "Config.h"
#include "RulesParser.h"

using namespace Sample;

const RulesManager& RulesManager::GetSingleton() noexcept {
    static RulesManager instance;

    static std::atomic_bool initialized;
    static std::latch latch(1);
    if (!initialized.exchange(true)) {
        for (const auto& dirEntry : std::filesystem::directory_iterator(RFSR_CONFIG_PATH)) {
            if (!dirEntry.is_regular_file() || dirEntry.path().extension() != ".toml") {
                continue;
            }
            log::info("Loading {}", dirEntry.path().string());
            try {
                auto config = toml::parse_file(dirEntry.path().string());
                if (const toml::array* _rules = config["rules"].as_array()) {
                    _rules->for_each([dirEntry](auto&& rule) mutable {
                        if constexpr (toml::is_table<decltype(rule)>) {
                            instance.rules.push_back(Rule::FromToml(rule));
                        } else {
                            throw std::invalid_argument("Error parsing rule config: invalid condition");
                        }
                    });
                }
            } catch (const toml::parse_error& err) {
                log::error("Failed to load {}: {}", dirEntry.path().string(), err.description());
            } catch (const std::exception& err) {
                log::error("Failed to load {}: {}", dirEntry.path().string(), err.what());
            }
        }
        std::sort(instance.rules.begin(), instance.rules.end());
        latch.count_down();
    }
    latch.wait();

    return instance;
}

[[nodiscard]] BGSFootstepSet* RulesManager::Apply(
    Actor* actor, BGSFootstepSet* orig_fss) const noexcept {
    auto cur_fss_ptr = orig_fss;
    for (const auto& rule : rules) {
        auto result = rule->Apply(actor, orig_fss);
        if (result.has_value() && rule->short_circuit) {
            return result.value();
        }
        cur_fss_ptr = result.value_or(cur_fss_ptr);
    }
    return cur_fss_ptr;
}