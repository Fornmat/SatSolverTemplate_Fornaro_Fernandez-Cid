/**
* @author Tim Luchterhand
* @date 27.11.24
* @brief Implementación del Solver SAT (DPLL + propagación unitaria) con heurística MostConstrained
*/

#include "Solver.hpp"        // Declaración de la clase Solver
#include "heuristics.hpp"    // Heurísticas de selección de variable (FirstVariable, MostConstrained, etc.)

#include <algorithm>         // std::count
#include <ranges>            // std::ranges::find_if
#include <utility>           // utilidades generales (por ejemplo std::move ya lo tienes en otros lados)

namespace sat {              // Espacio de nombres del solver

    // Constructor del solver: inicializa estructuras internas para numVariables variables
    Solver::Solver(unsigned numVariables)
    : numVars(numVariables),
      occCount(numVariables, 0u),
      model(numVariables, TruthValue::Undefined),
      heuristic(MostConstrained(&occCount)),
      watchlists(2u * numVariables),   // <-- importante: ids van de 0 a 2*numVars-1
      propagateHead(0) {}

    Solver::Solver(unsigned numVariables, FirstVariable h)
    : numVars(numVariables),
      occCount(numVariables, 0u),
      model(numVariables, TruthValue::Undefined),
      heuristic(h),
      watchlists(2u * numVariables),   // <-- importante: ids van de 0 a 2*numVars-1
      propagateHead(0) {}

    Solver::Solver(unsigned numVariables, RandomVariable h)
    : numVars(numVariables),
      occCount(numVariables, 0u),
      model(numVariables, TruthValue::Undefined),
      heuristic(h),
      watchlists(2u * numVariables),   // <-- importante: ids van de 0 a 2*numVars-1
      propagateHead(0) {}

    // Devuelve el valor (TruthValue) actual de una variable en el modelo
    TruthValue Solver::val(Variable x) const {
        return model[x.get()];                            // Accede al vector model usando el id de la variable
    }




    //El solver va a la cláusula, toma un literal, mira el valor actual de la variable asociada y 
    // comprueba si ese valor coincide con el signo del literal que se ha puesto como posible solución.
    // Si coinciden, el literal está satisfecho y devuelve true.

    // Comprueba si un literal está satisfecho con el modelo actual
    bool Solver::satisfied(Literal l) const {
        TruthValue v = val(var(l));                       // Obtiene el valor de la variable asociada al literal
        if (v == TruthValue::Undefined) return false;     // Si no está asignada, no se considera satisfecho

        // Si el literal es positivo y la variable es True -> satisfecho
        // Si el literal es negativo y la variable es False -> satisfecho
        return (l.sign() == +1 && v == TruthValue::True) ||
               (l.sign() == -1 && v == TruthValue::False);
    }

    // Comprueba si un literal está falsificado (equivalente a que su negación esté satisfecha)
    bool Solver::falsified(Literal l) const {
        return satisfied(l.negate());                     // Si ¬l está satisfecho, entonces l está falsificado
    }




    //Aquí se van añadiendo literales para ir construyendo la solución.
    //Si el literal ya estaba satisfecho, se deja todo como está.
    //Si no lo estaba, se asigna la variable correspondiente según el signo del literal 
    //y se guarda en el trail para poder volver atrás si hace falta. 

    // Asigna un literal al modelo (añade también al trail unitLiterals)
    bool Solver::assign(Literal l) {
        if (falsified(l)) return false;                   // Si ya contradice el modelo, asignar falla
        if (satisfied(l)) return true;                    // Si ya estaba satisfecho, no hace falta cambiar nada

        // Asigna la variable según el signo del literal
        model[var(l).get()] = (l.sign() == +1) ? TruthValue::True : TruthValue::False;

        unitLiterals.push_back(l);                        // Guarda en el trail (decisiones + propagaciones)
        return true;                                      // Asignación correcta
    }




    // Añade una cláusula al solver
    bool Solver::addClause(Clause clause) {
        // Si la cláusula está vacía, es un conflicto inmediato → no se puede añadir
        if (clause.isEmpty()) return false;
        // Si la cláusula es unitaria (solo un literal),
        // ese literal debe ser verdadero obligatoriamente
        if (clause.size() == 1) {
            return assign(clause[0]); // unit clause ⇒ se asigna directamente al modelo
        }
        // Guardamos la cláusula en el vector 'clauses'
        // Usamos shared_ptr para no copiar la cláusula y poder referenciarla desde varios sitios
        clauses.push_back(std::make_shared<Clause>(std::move(clause)));
        // Obtenemos un puntero a la cláusula recién añadida
        ClausePointer cp = clauses.back();
        // Referencia cómoda a la cláusula real (para leer sus literales)
        const Clause& c = *cp;
        // Recorremos todos los literales de la cláusula
        for (Literal l : c) {
            // Incrementamos el número de veces que aparece la variable del literal
            // Esto se usa para elegir la variable más restringida
            ++occCount[var(l).get()];
        }

        // Obtenemos los dos literales vigilados de la cláusula
        // Rank 0 y rank 1 son los dos watchers iniciales
        Literal w0 = c.getWatcherByRank(0);
        Literal w1 = c.getWatcherByRank(1);

        // Añadimos la cláusula a la lista de watchers del literal w0
        // watchlists[index] = cláusulas que vigilan ese literal
        watchlists[litIndex(w0)].push_back(cp);

        // Hacemos lo mismo para el segundo watcher w1
        watchlists[litIndex(w1)].push_back(cp);

        // La cláusula se añadió correctamente
        return true;
    }





    // Devuelve una versión "reducida" de las cláusulas según el modelo actual
    // Elimina cláusulas ya satisfechas y borra los literales falsificados de las demás

    auto Solver::rebase() const -> std::vector<Clause> {
        std::vector<Clause> reducedClauses;               // Lista de cláusulas simplificadas (para tests)

        for (const auto &cptr : clauses) {                // Recorre todas las cláusulas almacenadas
            const Clause &c = *cptr;                      // Referencia a la cláusula real
            bool satClause = false;                       // Flag: ¿cláusula ya satisfecha?
            std::vector<Literal> newLits;                 // Literales que quedan tras eliminar falsificados

            for (Literal l : c) {                         // Recorre literales de la cláusula
                if (satisfied(l)) {                       // Si algún literal está satisfecho...
                    satClause = true;                     // ...toda la cláusula se satisface
                    break;                                // Ya no necesitamos procesarla
                }
                if (!falsified(l)) {                      // Si el literal no está falsificado...
                    newLits.push_back(l);                 // ...se mantiene en la cláusula reducida
                }
            // Devuelve una versión "reducida" de las cláusulas según el modelo actual
            // Elimina cláusulas ya satis
            }

            if (!satClause) {                             // Si la cláusula NO estaba satisfecha
                Clause newClause(std::move(newLits));     // Creamos la cláusula reducida
                auto it = std::ranges::find_if(           // Comprobamos si ya existe una igual (sin importar orden)
                    reducedClauses,
                    [&newClause](const Clause &other) {
                        return other.sameLiterals(newClause);
                    }
                );

                if (it == reducedClauses.end()) {         // Si no existía aún...
                    reducedClauses.push_back(std::move(newClause)); // ...la añadimos
                }
            }
        }

        // Las unit literals se devuelven como cláusulas unitarias (lo esperan los tests)
        for (Literal l : unitLiterals) {
            reducedClauses.emplace_back(std::vector{l});  // Cada literal del trail se mete como {l}
        }

        return reducedClauses;                            // Devuelve el conjunto reducido
    }




    // Propagación unitaria (versión naive): repite hasta que ya no haya cambios
    bool Solver::unitPropagate() {
    // Propagamos solo lo nuevo del trail
    while (propagateHead < unitLiterals.size()) { //propagatedHead hasta donde ha propagado
        Literal assigned = unitLiterals[propagateHead++];

        // El literal que se vuelve falso por esta asignación es su negación
        Literal falsifiedLit = assigned.negate(); //si x verdadero no x falso
        auto& wl = watchlists[litIndex(falsifiedLit)]; // los q contienen el falso se reducen 

        // Recorremos SOLO las cláusulas que estaban vigilando falsifiedLit
        std::size_t i = 0;
        while (i < wl.size()) {
            ClausePointer cp = wl[i]; //te da un puntero para apuntar a una clausula
            Clause& c = *cp; // lo derefeerencias para trabajar con ella directalmùente

            // ¿Qué watcher (0 o 1) es el que coincide con falsifiedLit?
            short rank = c.getRank(falsifiedLit);
            if (rank == -1) {
                // Si por alguna razón no lo vigila (puede pasar si ya se movió),
                // lo saltamos eliminándolo de esta lista.
                wl[i] = wl.back();
                wl.pop_back();
                continue;
            }

            short otherRank = (rank == 0) ? 1 : 0; //si watcher es 0 coge el 1 sino 0
            Literal other = c.getWatcherByRank(otherRank);

            // Si el otro watcher ya satisface la cláusula -> cláusula OK
            if (satisfied(other)) {
                ++i;
                continue;
            }

            // Intentar mover el watcher rank a otro literal que no esté falsificado
            bool moved = false;
            for (Literal cand : c) {
    // Devuelve una versión "reducida" de las cláusulas según el modelo actual
    // Elimina cláusulas ya satis
                if (cand == other) continue;          //puedes poner los dos watchers sobre el mismo literal (salvo unit clause).
                if (!falsified(cand)) {               // si candidato “vivo”, es decir no falsificado
                    c.setWatcher(cand, rank);  //ctualizas la cláusula, mover el watcher a ella a cand

                    // quitar de esta watchlist (swap-pop) porq ya no vigilamos ese literal
                    wl[i] = wl.back();
                    wl.pop_back();

                    // añadir a la watchlist del nuevo watcher, osea vigilamos la clausula de cand
                    watchlists[litIndex(cand)].push_back(cp);

                    moved = true;
                    break;
                }
            }

            if (moved) {
                // OJO: no hacemos i++ porque hemos hecho swap-pop (wl[i] cambió)
                continue;
            }

            // No se pudo mover: entonces la cláusula es UNIT o CONFLICT
            if (falsified(other)) {
                // Ambos watchers falsificados -> conflicto
                return false;
            }

            // Si el otro no está falsificado ni satisfecho => debe estar Undefined => unit
            if (!assign(other)) {
                return false;
            }

            // La cláusula sigue vigilando falsifiedLit (aunque esté falsificado), por eso la dejamos aquí
            ++i;
        }
    }

    return true;
}

    // Cuenta cuántas variables siguen sin asignar (Undefined)
    std::size_t Solver::countOpenVariables() const {
        return static_cast<std::size_t>(
            std::count(model.begin(), model.end(), TruthValue::Undefined) // cuenta Undefined en el vector model
        );
    }

    // Comprueba si todas las variables están asignadas
    bool Solver::allAssigned() const {
        return countOpenVariables() == 0;                 // si no quedan Undefined, todo asignado
    }

    // Crea un nuevo nivel de decisión (guardar el tamaño del trail)
    void Solver::newDecisionLevel() {
        trailLimits.push_back(unitLiterals.size());       // guardamos hasta dónde llega el trail en este nivel
    }

    // Vuelve atrás un nivel: deshace asignaciones hasta el límite del nivel anterior
    void Solver::backtrackOneLevel() {
        std::size_t limit = trailLimits.back();           // límite del trail para este nivel
        trailLimits.pop_back();                           // eliminamos el nivel actual

        while (unitLiterals.size() > limit) {             // mientras haya literales por encima del límite
            Literal l = unitLiterals.back();              // cogemos el último literal asignado
            unitLiterals.pop_back();                      // lo quitamos del trail
            model[var(l).get()] = TruthValue::Undefined;  // desasignamos esa variable en el modelo
        }

        // Si usamos propagación con watchers + "propagateHead", al hacer backtracking el trail se acorta,
        // así que debemos asegurarnos de que propagateHead no apunte más allá del nuevo tamaño del trail.
        if (propagateHead > unitLiterals.size()) {        // si el puntero de propagación quedó "fuera" (hace la vuelta)
            propagateHead = unitLiterals.size();          // lo bajamos al final del trail actual
        }
    }



    // Función principal de resolución (DPLL recursivo) ARBOOLL
    bool Solver::solve() {
        if (!unitPropagate()) return false;               // primero propagación unitaria; si conflicto -> UNSAT
        if (allAssigned()) return true;                   // si todo asignado sin conflicto -> SAT

        // choose variable using MostConstrained (stored in heuristic)
        Variable x = heuristic(model, countOpenVariables()); // selecciona variable no asignada con más ocurrencias

        for (Literal decision : {pos(x), neg(x)}) {       // probamos primero x=true (pos) y luego x=false (neg)
            newDecisionLevel();                           // abrimos nivel de decisión (para poder backtrack)

            if (assign(decision) && solve()) {            // asignamos la decisión y seguimos recursivamente
                return true;                              // si encuentra SAT en esa rama, terminamos
            }

            backtrackOneLevel();                          // si falla, deshacemos y probamos la otra rama
        }

        return false;                                     // si ambas ramas fallan -> UNSAT
    }

        Variable Solver::pickMostConstrainedTiebreakRandom(std::mt19937 &rng) const {
            unsigned bestScore = 0;
            bool found = false;

            // 1) Find best score among unassigned vars
            for (unsigned varId = 0; varId < model.size(); ++varId) {
                if (model[varId] != TruthValue::Undefined) continue;
                const unsigned score = occCount[varId];
                if (!found || score > bestScore) {
                    found = true;
                    bestScore = score;
                }
            }

            if (!found) {
                // fallback to existing heuristic (should not happen if not allAssigned)
                return heuristic(model, countOpenVariables());
            }

            // 2) Build candidate set: "near best" (>= 90% of bestScore)
            // This makes tie-break less rigid and helps avoid deterministic traps.
            const unsigned threshold = (bestScore * 9u) / 10u;

            std::vector<unsigned> candidates;
            candidates.reserve(model.size());

            for (unsigned varId = 0; varId < model.size(); ++varId) {
                if (model[varId] != TruthValue::Undefined) continue;
                const unsigned score = occCount[varId];
                if (score >= threshold) {
                    candidates.push_back(varId);
                }
            }

            // If something weird happens, fallback
            if (candidates.empty()) {
                return Variable(0);
            }

            std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
            return Variable(candidates[dist(rng)]);
        }


    SolveStatus Solver::solveRec(std::mt19937 &rng) {
        if (!unitPropagate()) {
            ++conflicts;
            if (conflicts >= restartBudget) return SolveStatus::RESTART;
            return SolveStatus::UNSAT;
        }

        if (allAssigned()) return SolveStatus::SAT;

        const Variable x = pickMostConstrainedTiebreakRandom(rng);

        // Random polarity: sometimes try neg first
        bool startWithPos = true;
        {
            std::uniform_int_distribution<int> coin(0, 1);
            startWithPos = (coin(rng) == 1);
        }

        Literal first = startWithPos ? pos(x) : neg(x);
        Literal second = startWithPos ? neg(x) : pos(x);

        for (Literal decision : {first, second}) {
            newDecisionLevel();

            if (assign(decision)) {
                SolveStatus st = solveRec(rng);
                if (st == SolveStatus::SAT) return SolveStatus::SAT;
                if (st == SolveStatus::RESTART) return SolveStatus::RESTART;
            }

            backtrackOneLevel();
        }

        return SolveStatus::UNSAT;
    }


    bool Solver::solve_with_restart() {
        // RNG (fixed seed would make runs reproducible; random_device makes it different each time)
        std::random_device rd;
        std::mt19937 rng(rd());

        while (true) {
            SolveStatus st = solveRec(rng);

            if (st == SolveStatus::SAT) return true;
            if (st == SolveStatus::UNSAT) return false;

            // RESTART: backtrack to level 0
            while (!trailLimits.empty()) backtrackOneLevel();

            // Increase budget and reset conflicts counter
            restartBudget *= restartFactor;
            conflicts = 0;
        }
    }

    // Devuelve el trail (literales asignados) como referencia constante
    auto Solver::getUnitLiterals() const -> const std::vector<Literal>& {
        return unitLiterals;                              // permite imprimir la solución o usarla en tests
    }

} // namespace sat
