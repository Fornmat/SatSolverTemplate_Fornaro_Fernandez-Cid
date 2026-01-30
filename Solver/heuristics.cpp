/**
* @author Tim Luchterhand
* @date 29.11.24
* @brief
*/

#include <Iterators.hpp>

#include "heuristics.hpp"
#include "util/exception.hpp"

namespace sat {

    Variable FirstVariable::operator()(const std::vector<TruthValue> &model, std::size_t) const {
        for (auto [varId, val]: iterators::enumerate(model, 0u)) {
            if (val == TruthValue::Undefined) {
                return Variable(varId);
            }
        }
        throw std::runtime_error("Found no open variable");
    }

    Variable MostConstrained::operator()(const std::vector<TruthValue> &model, std::size_t) const {
        // Comprobación de seguridad: el puntero a occCount debe existir
        // occ apunta al vector que cuenta cuántas veces aparece cada variable
        if (occ == nullptr) {
            throw std::runtime_error("MostConstrained: occ pointer is null");
        }

        unsigned bestVar = 0;        // Variable candidata con más restricciones
        unsigned bestScore = 0;      // Número máximo de ocurrencias encontrado
        bool found = false;          // Indica si hemos encontrado alguna variable no asignada

        // Recorremos todas las variables del modelo
        for (unsigned varId = 0; varId < model.size(); ++varId) {

            // Si la variable ya está asignada (True o False), la ignoramos
            if (model[varId] != TruthValue::Undefined) continue;

            //sino (n de ocurrencias), Número de ocurrencias de esta variable en las cláusulas
            const unsigned score = (*occ)[varId];

            // Si es la primera variable libre encontrada
            // o si tiene más ocurrencias que la mejor hasta ahora
            if (!found || score > bestScore) {
                found = true;        // Ya tenemos al menos una variable candidata
                bestScore = score;  // Actualizamos el mejor número de ocurrencias
                bestVar = varId;    // Guardamos el identificador de la variable
            }
        }

        // Si no se encontró ninguna variable sin asignar, es un error lógico
        // (normalmente significa que ya estaban todas asignadas)
        if (!found) {
            throw std::runtime_error("Found no open variable");
        }

        // Devolvemos la variable con más ocurrencias (más "constrained")
        return Variable(bestVar);
    }

    Variable RandomVariable::operator()(const std::vector<TruthValue> &model, std::size_t numOpenVariables) const {
        if (numOpenVariables == 0u) {
            throw std::runtime_error("Found no open variable");
        }

        std::uniform_int_distribution<std::size_t> dist(0u, numOpenVariables - 1u);
        std::size_t target = dist(rng);

        std::size_t count = 0;
        for (unsigned varId = 0; varId < model.size(); ++varId) {
            if (model[varId] == TruthValue::Undefined) {
                if (count == target) {
                    return Variable(varId);
                }
                ++count;
            }
        }

        throw std::runtime_error("RandomVariable: inconsistent open variable count");
    }


    Variable Heuristic::operator()(const std::vector<TruthValue> &values, std::size_t numOpenVariables) const {
        if (nullptr == impl) {
            throw BadHeuristicCall("heuristic wrapper does not contain a heuristic");
        }
        return impl->invoke(values, numOpenVariables);
    }

    bool Heuristic::isValid() const {
        return nullptr != impl;
    }

} // namespace sat
