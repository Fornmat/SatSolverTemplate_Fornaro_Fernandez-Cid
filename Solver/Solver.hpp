/**
* @author Tim Luchterhand
* @date 27.11.24
* @file Solver.hpp
* @brief Contains the main solver class
*/

#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <memory>
#include <vector>
#include <stdexcept> // std::exception

#include "heuristics.hpp"
#include "basic_structures.hpp"
#include "Clause.hpp"

namespace sat {

    using ClausePointer = std::shared_ptr<Clause>;
    using ConstClausePointer = std::shared_ptr<const Clause>;
    enum class SolveStatus { SAT, UNSAT, RESTART };

    class Solver {
    private:

        
        unsigned numVars = 0;

        // occurrences per variable (0..n-1)
        std::vector<unsigned> occCount;

        // assignment per variable (0..n-1)
        std::vector<TruthValue> model;

        // heuristic wrapper (MostConstrained por defecto en el ctor)
        Heuristic heuristic;

        // Watchlists: por cada literal-id guardamos las cláusulas que lo vigilan
        std::vector<std::vector<ClausePointer>> watchlists;

        // Índice del trail ya propagado (para no re-propagar todo cada vez)
        std::size_t propagateHead = 0;

        // Helper: map literal -> index (en tu caso ya es el id interno)
        std::size_t litIndex(Literal l) const { return static_cast<std::size_t>(l.get()); }

        // CNF clauses
        std::vector<ClausePointer> clauses;

        // trail: decisions + propagations
        std::vector<Literal> unitLiterals;

        // backtracking limits (marca el tamaño del trail en cada nivel de decisión)
        std::vector<std::size_t> trailLimits;

        unsigned conflicts = 0;
        unsigned restartBudget = 500;
        unsigned restartFactor = 2;

        // Helpers
        std::size_t countOpenVariables() const;
        bool allAssigned() const;
        void newDecisionLevel();
        void backtrackOneLevel();

        Variable pickMostConstrainedTiebreakRandom(std::mt19937 &rng) const;
        SolveStatus solveRec(std::mt19937 &rng);

    public:
        explicit Solver(unsigned numVariables);
        Solver(unsigned numVariables, FirstVariable h);
        Solver(unsigned numVariables, RandomVariable h);

        bool addClause(Clause clause);

        auto rebase() const -> std::vector<Clause>;

        TruthValue val(Variable x) const;

        bool satisfied(Literal l) const;

        bool falsified(Literal l) const;

        bool assign(Literal l);

        bool unitPropagate();

        auto getUnitLiterals() const -> const std::vector<Literal>&;

        bool solve();
        
        // Solve “externo”: maneja los restarts y llama a solveRec()
        bool solve_with_restart();

        const std::vector<unsigned>& getOccCount() const { return occCount; }
    };

} // namespace sat

#endif // SOLVER_HPP
