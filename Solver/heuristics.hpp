/**
* @author Tim Luchterhand
* @date 29.11.24
* @file heuristics.hpp
* @brief Contains different branching heuristics
*/

#ifndef HEURISTICS_HPP
#define HEURISTICS_HPP

#include <vector>
#include <memory>
#include <random>

#include "basic_structures.hpp"
#include "util/concepts.hpp"

namespace sat {

    template<typename H>
    concept heuristic = concepts::callable_r<H, Variable, const std::vector<TruthValue>, std::size_t>;

    /**
     * @brief Selects the first unassigned variable
     */
    struct FirstVariable {
        Variable operator()(const std::vector<TruthValue> &model, std::size_t) const;
    };

    /**
     * @brief Selects an unassigned variable with the most occurrences in the formula.
     */
    struct MostConstrained {
        const std::vector<unsigned>* occ = nullptr;

        explicit MostConstrained(const std::vector<unsigned>* occCounts) : occ(occCounts) {}

        Variable operator()(const std::vector<TruthValue>& model, std::size_t) const;
    };

    struct RandomVariable {
        mutable std::mt19937 rng{std::random_device{}()};

        Variable operator()(const std::vector<TruthValue>& model, std::size_t numOpenVariables) const;
    };

    namespace detail {
        struct HeuristicCallableBase {
            HeuristicCallableBase() = default;
            virtual ~HeuristicCallableBase() = default;

            HeuristicCallableBase(HeuristicCallableBase &&) = default;
            HeuristicCallableBase &operator=(HeuristicCallableBase &&) = default;
            HeuristicCallableBase(const HeuristicCallableBase &) = default;
            HeuristicCallableBase &operator=(const HeuristicCallableBase &) = default;

            virtual Variable invoke(const std::vector<TruthValue> &, std::size_t) = 0;
        };

        template<heuristic H>
        struct HeuristicCallable : HeuristicCallableBase {
            H impl;

            template<typename... Args>
            explicit HeuristicCallable(Args &&... args): impl(std::forward<Args>(args)...) {}

            Variable invoke(const std::vector<TruthValue> &values, std::size_t numOpenVariables) override {
                return impl(values, numOpenVariables);
            }
        };
    }

    class Heuristic {
        std::unique_ptr<detail::HeuristicCallableBase> impl;
    public:
        Heuristic() = default;

        template<heuristic H>
        Heuristic(H &&heuristic): impl(
            std::make_unique<detail::HeuristicCallable<std::remove_cvref_t<H>>>(std::forward<H>(heuristic))) {}

        Variable operator()(const std::vector<TruthValue> &values, std::size_t numOpenVariables) const;

        bool isValid() const;
    };

} // namespace sat

#endif // HEURISTICS_HPP
