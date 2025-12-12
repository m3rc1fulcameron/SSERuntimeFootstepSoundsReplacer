#pragma once

#include <toml++/toml.h>

using namespace RE;
using namespace SKSE;

namespace Sample {
    class BasePredicate {
    protected:
        const bool invert;

    public:
        [[nodiscard]] static const std::shared_ptr<BasePredicate> FromToml(const toml::table&);
        [[nodiscard]] virtual inline bool Evaluate(Actor*, const BGSFootstepSet*) const noexcept {
            log::error("Tried to Evaluate() a BasePredicate");
            return false;
        };
        BasePredicate(bool invert) : invert{invert} {};
    };

    class ActorIsFemalePredicate : public BasePredicate {
    public:
        [[nodiscard]] inline bool Evaluate(Actor*, const BGSFootstepSet*) const noexcept;
        ActorIsFemalePredicate(bool invert) : BasePredicate(invert) {}
    };

    class ActorHasKeywordPredicate : public BasePredicate {
    private:
        const std::string keyword;

    public:
        [[nodiscard]] inline bool Evaluate(Actor*, const BGSFootstepSet*) const noexcept;
        ActorHasKeywordPredicate(bool invert, const std::string keyword) : BasePredicate(invert), keyword{keyword} {};
    };

    class ArmorHasKeywordPredicate : public BasePredicate {
    private:
        const BGSBipedObjectForm::BipedObjectSlot slot;
        const std::string keyword;

    public:
        [[nodiscard]] inline bool Evaluate(Actor*, const BGSFootstepSet*) const noexcept;
        ArmorHasKeywordPredicate(bool invert, BGSBipedObjectForm::BipedObjectSlot slot, const std::string keyword)
            : BasePredicate(invert), slot{slot}, keyword{keyword} {};
    };

    class FootstepSetFormIDPredicate : public BasePredicate {
    private:
        const BGSFootstepSet* fss;

    public:
        [[nodiscard]] inline bool Evaluate(Actor*, const BGSFootstepSet*) const noexcept;
        FootstepSetFormIDPredicate(bool invert, const BGSFootstepSet* fss) : BasePredicate(invert), fss{fss} {};
    };

    class CompoundPredicate : public BasePredicate {
    public:
        enum Conjunction { AND, OR };

    private:
        std ::vector<std::shared_ptr<BasePredicate>> predicates;

    public:
        const Conjunction conjunction;
        [[nodiscard]] inline bool Evaluate(Actor*, const BGSFootstepSet*) const noexcept;
        CompoundPredicate(bool invert, Conjunction conjunction,
                          const std::vector<std::shared_ptr<BasePredicate>> predicates)
            : BasePredicate(invert), conjunction{conjunction}, predicates(predicates) {}
    };

    class Rule {
    public:
        const std::string name;
        const int priority;
        BGSFootstepSet* replacement;
        const bool short_circuit;
        const enum EvalMode { ANY, ALL } eval_mode;
        const std::vector<std::shared_ptr<BasePredicate>> conditions;
        [[nodiscard]] static const std::shared_ptr<Rule> FromToml(const toml::table&);
        [[nodiscard]] const std::optional<BGSFootstepSet*> Apply(Actor*, const BGSFootstepSet*) const noexcept;
        [[nodiscard]] bool operator<(const Rule& r) const noexcept { return priority < r.priority; }
        Rule(const std::string name, int priority, BGSFootstepSet* replacement, bool short_circuit, EvalMode eval_mode,
             const std::vector<std::shared_ptr<BasePredicate>> conditions)
            : name{name},
              priority{priority},
              replacement{replacement},
              short_circuit{short_circuit},
              eval_mode{eval_mode},
              conditions(conditions){};
    };
}  // namespace Sample