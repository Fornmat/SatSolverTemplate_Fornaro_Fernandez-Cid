/**
* @author Tim Luchterhand
* @date 26.11.24
* @file Clause.hpp
* @brief Contains the class Clause that consists of one or more literals
*/

#ifndef CLAUSE_HPP
#define CLAUSE_HPP

#include <vector>
#include <ostream>

#include "util/concepts.hpp"
#include "basic_structures.hpp"

namespace sat {

    template<typename T>
    concept clause_like = concepts::typed_range<T, Literal>;

    /**
     * @brief Clause class with watch literals.
     */
    class Clause {
    private:
        std::vector<Literal> _lits;
        std::size_t _watch0 = 0;
        std::size_t _watch1 = 0;

    public:
        Clause() = default;

        /**
         * CTor
         * @param literals list of literals of the clause
         */
        Clause(std::vector<Literal> literals);

        short getRank(Literal l) const;

        std::size_t getIndex(short rank) const;

        bool setWatcher(Literal l, short watcherNo);

        Literal getWatcherByRank(short rank) const;

        auto begin() const -> std::vector<Literal>::const_iterator;
        auto end() const -> std::vector<Literal>::const_iterator;

        Literal operator[](std::size_t index) const;

        bool isEmpty() const;

        std::size_t size() const;

        bool sameLiterals(const Clause &other) const;
    };
}

#endif // CLAUSE_HPP
