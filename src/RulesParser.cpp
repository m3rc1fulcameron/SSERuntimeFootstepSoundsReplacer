#include "RulesParser.h"

using namespace Sample;

void TokenizePath(const std::string_view& config_str,
                                                               std::vector<std::string_view>& tokens,
                                                               const std::string_view& delimiter = "."sv) noexcept {
    size_t start = 0;
    size_t pos = 0;
    while ((pos = config_str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(config_str.substr(start, pos-start));
        start = pos + 1;
    }
    tokens.push_back(config_str.substr(start, config_str.size()-start));
}

const std::map<std::string_view, BGSBipedObjectForm::BipedObjectSlot> slot_map = {
    {"feet"sv, BGSBipedObjectForm::BipedObjectSlot::kFeet}};

[[nodiscard]] const std::shared_ptr<BasePredicate> BasePredicate::FromToml(const toml::table& pred_config) {
    log::info("Importing predicate");
    auto tokens = std::vector<std::string_view>();
    TokenizePath(pred_config["pred"].value<std::string_view>().value(), tokens);
    if (tokens.size() < 1) {
        throw std::invalid_argument("Pred path length < 1");
    }
    auto invert = pred_config["not"].value_or(false);
    if (tokens[0] == "actor" && tokens.size() >= 2) {
        log::trace("Actor");
        if (tokens[1] == "isFemale") {
            return std::make_shared<ActorIsFemalePredicate>(invert);
        } else if (tokens[1] == "hasKeyword") {
            auto keyword = pred_config["keyword"].value<std::string>().value();
            return std::make_shared<ActorHasKeywordPredicate>(invert, keyword);
        } else {
            throw std::invalid_argument("Invalid path for actor subject");
        }
    } else if (tokens[0] == "armor" && tokens.size() >= 3) {
        log::trace("Armor");
        auto slot = slot_map.at(tokens[1]);
        if (tokens[2] == "hasKeyword") {
            auto keyword = pred_config["keyword"].value<std::string>().value();
            return std::make_shared<ArmorHasKeywordPredicate>(invert, slot, keyword);
        } else {
            throw std::invalid_argument("Invalid path for armor subject");
        }
    } else if (tokens[0] == "footstepSet") {
        log::trace("footstepSet");
        auto mod = pred_config["mod"].value<std::string_view>().value();
        auto formID = pred_config["formID"].value<FormID>().value();
        static const auto handler = TESDataHandler::GetSingleton();
        auto fss = handler->LookupForm<BGSFootstepSet>(formID, mod);
        if (!fss) {
            throw std::invalid_argument(fmt::format("Could not find fss {}+{}", mod, formID));
        }
        return std::make_shared<FootstepSetFormIDPredicate>(invert, fss);
    } else if (tokens[0] == "AND" || tokens[0] == "OR") {
        log::trace("AND/OR");
        auto conjunction = tokens[0] == "AND"  ? CompoundPredicate::Conjunction::AND
                           : tokens[0] == "OR" ? CompoundPredicate::Conjunction::OR
                                               : throw std::invalid_argument("Invalid conjunction");
        auto predicates = std::vector<std::shared_ptr<BasePredicate>>();
        if (const toml::array* _predicates = pred_config["preds"].as_array()) {
            _predicates->for_each([&predicates](auto&& predicate) mutable {
                if constexpr (toml::is_table<decltype(predicate)>) {
                    predicates.push_back(BasePredicate::FromToml(predicate));
                } else {
                    throw std::invalid_argument("Error parsing rule config: invalid condition");
                }
            });
        }
        log::debug("Initialized {} children", predicates.size());
        return std::make_shared<CompoundPredicate>(invert, conjunction, predicates);
    } else {
        throw std::invalid_argument("Invalid pred subject");
    }
}

[[nodiscard]] inline bool ActorIsFemalePredicate::Evaluate(Actor* actor,
                                                           const BGSFootstepSet* orig_fss) const noexcept {
    return (actor && orig_fss && actor->GetActorBase()->IsFemale()) != invert;
}

[[nodiscard]] inline bool ActorHasKeywordPredicate::Evaluate(Actor* actor,
                                                             const BGSFootstepSet* orig_fss) const noexcept {
    return (actor && orig_fss && actor->HasKeywordString(keyword)) != invert;
}

[[nodiscard]] inline bool ArmorHasKeywordPredicate::Evaluate(Actor* actor,
                                                             const BGSFootstepSet* orig_fss) const noexcept {
    if (!actor) {
        return false;
    }
    auto armor = actor->GetWornArmor(slot);
    return (orig_fss && armor && armor->HasKeywordString(keyword)) != invert;
}

[[nodiscard]] inline bool FootstepSetFormIDPredicate::Evaluate(Actor* actor, const BGSFootstepSet* orig_fss) const noexcept {
    return (actor && orig_fss && orig_fss->formID == fss->formID) != invert;
}

[[nodiscard]] inline bool CompoundPredicate::Evaluate(Actor* actor, const BGSFootstepSet* orig_fss) const noexcept {
    for (auto& pred : predicates) {
        auto result = pred->Evaluate(actor, orig_fss);
        if (conjunction == OR && result) {
            return true != invert;
        } else if (conjunction == AND && !result) {
            return false != invert;
        }
    }
    return conjunction == AND ? true != invert : false != invert;
}

[[nodiscard]] const std::shared_ptr<Rule> Rule::FromToml(const toml::table& rule_config) {
    log::info("Importing rule: {}", rule_config["name"].value<std::string>().value());
    auto eval_mode = rule_config["evaluate"].value_or("all") == "all" ? Rule::EvalMode::ALL : Rule::EvalMode::ALL;
    static auto dataHandler = TESDataHandler::GetSingleton();
    auto replacement_mod = rule_config["replacement"]["mod"].value<std::string>().value();
    auto replacement_formID = rule_config["replacement"]["formID"].value<FormID>().value();
    auto replacement = dataHandler->LookupForm<BGSFootstepSet>(replacement_formID, replacement_mod);
    if (!replacement) {
        throw std::invalid_argument("Could not find replacement");
    }
    auto conditions = std::vector<std::shared_ptr<BasePredicate>>();
    if (const toml::array* _conditions = rule_config["conditions"].as_array()) {
        _conditions->for_each([&conditions](auto&& condition) mutable {
            if constexpr (toml::is_table<decltype(condition)>) {
                conditions.push_back(BasePredicate::FromToml(condition));
            } else {
                throw std::invalid_argument("Error parsing rule config: invalid condition");
            }
        });
    }
    log::info("Initialized {} conditions", conditions.size());
    return std::make_shared<Rule>(rule_config["name"].value<std::string>().value(), rule_config["priority"].value_or(0),
                                  replacement, rule_config["short_circuit"].value_or(false), eval_mode, conditions);
}

[[nodiscard]] const std::optional<BGSFootstepSet*> Rule::Apply(Actor* actor,
                                                                     const BGSFootstepSet* orig_fss) const noexcept {
    log::debug("Evaluating {}", name);
    for (auto& pred : conditions) {
        auto result = pred->Evaluate(actor, orig_fss);
        log::debug("Evaluated to {}", result);
        if (eval_mode == ANY && result) {
            log::trace("Short circuiting ANY condition: true");
            return std::optional<BGSFootstepSet*>{replacement};
        } else if (eval_mode == ALL && !result) {
            log::trace("Short circuiting ALL condition: false");
            return std::nullopt;
        }
    }
    log::trace("Rule fell through");
    return eval_mode == ALL ? std::optional<BGSFootstepSet*>{replacement} : std::nullopt;
}