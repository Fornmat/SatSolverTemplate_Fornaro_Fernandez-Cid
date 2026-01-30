/**
* @author Tim Luchterhand
* @date 26.11.24
* @brief Implementación de la clase Clause con dos watchers (watched literals)
*/

#include <cassert>      // Para usar assert() y validar condiciones en tiempo de ejecución
#include <algorithm>    // Para std::find, std::sort, std::distance

#include "Clause.hpp"   // Declaración de la clase Clause

namespace sat {         // Espacio de nombres del solver SAT

    // Constructor: recibe una lista de literales y la guarda en _lits
    Clause::Clause(std::vector<Literal> literals)
        : _lits(std::move(literals)) {   // move para evitar copiar el vector

        // Si la cláusula está vacía, dejamos los watchers en 0 (no hay literales válidos)
        if (_lits.empty()) {
            _watch0 = 0;                 // watcher 0 apunta a "0" por defecto
            _watch1 = 0;                 // watcher 1 apunta a "0" por defecto
        }
        // Si la cláusula tiene un solo literal, ambos watchers apuntan al mismo literal (cláusula unitaria)
        else if (_lits.size() == 1) {
            _watch0 = 0;                 // watcher 0 observa el único literal
            _watch1 = 0;                 // watcher 1 también observa el mismo literal
        }
        // Si hay 2 o más literales, inicializamos watchers en las dos primeras posiciones
        else {
            _watch0 = 0;                 // watcher 0 observa _lits[0]
            _watch1 = 1;                 // watcher 1 observa _lits[1]
        }
    }

    // Devuelve el "rank" de un literal: 0 si es watcher0, 1 si es watcher1, -1 si no es watcher
    short Clause::getRank(Literal l) const {
        if (_lits.empty()) return -1;        // si no hay literales, no puede haber watcher válido
        if (_lits[_watch0] == l) return 0;   // si coincide con watcher 0, rank 0
        if (_lits[_watch1] == l) return 1;   // si coincide con watcher 1, rank 1
        return -1;                            // no es ninguno de los dos watchers
    }

    // Devuelve el índice del watcher de lal ista de literales correspondiente al rank (0 o 1), 
    std::size_t Clause::getIndex(short rank) const {
        assert(rank == 0 || rank == 1);      // rank debe ser 0 o 1 (si no, es un bug)
        return (rank == 0) ? _watch0 : _watch1;  // devuelve el índice correcto
    }

    // Cambia uno de los watchers para que apunte al literal l
    bool Clause::setWatcher(Literal l, short watcherNo) {
        assert(watcherNo == 0 || watcherNo == 1);   // watcherNo debe ser 0 o 1

        // Buscamos el literal l dentro del vector de literales
        auto it = std::find(_lits.begin(), _lits.end(), l);
        if (it == _lits.end()) return false;        // si no existe en la cláusula, no se puede poner como watcher

        // Calculamos el índice del literal encontrado en el vector
        std::size_t idx = static_cast<std::size_t>(std::distance(_lits.begin(), it));

        // Actualizamos el watcher correspondiente para que apunte a ese índice
        if (watcherNo == 0) _watch0 = idx;          // cambiar watcher0
        else _watch1 = idx;                         // cambiar watcher1

        return true;                                 // actualización correcta si existia ese literal
    }

    //PARA RECORRER UNA CLAUSULA COMO SI FUESE UNA LISTA
    // Devuelve un iterador al primer literal (permite usar range-based for)
    auto Clause::begin() const -> std::vector<Literal>::const_iterator {
        return _lits.begin();                        // inicio del vector
    }

    // Devuelve un iterador al "pasado-el-final" (permite usar range-based for)
    auto Clause::end() const -> std::vector<Literal>::const_iterator {
        return _lits.end();                          // fin del vector
    }

    // Comprueba si la cláusula está vacía
    bool Clause::isEmpty() const {
        return _lits.empty();                        // true si no hay literales
    }

    // Operador [] para acceder a un literal por índice
    Literal Clause::operator[](std::size_t index) const {
        assert(index < _lits.size());                // evita acceso fuera de rango
        return _lits[index];                         // devuelve el literal en esa posición
    }

    // Devuelve el número de literales de la cláusula
    std::size_t Clause::size() const {
        return _lits.size();                         // tamaño del vector de literales
    }

    // Devuelve el literal observado por rank 0 o rank 1
    Literal Clause::getWatcherByRank(short rank) const {
        assert(rank == 0 || rank == 1);              // rank solo puede ser 0 o 1
        assert(!_lits.empty());                      // no tiene sentido pedir watchers si está vacía
        return _lits[getIndex(rank)];                // devuelve el literal en el índice del watcher
    }

    // Comprueba si dos cláusulas contienen los mismos literales (sin importar el orden) te da vrai si son igales
    bool Clause::sameLiterals(const Clause &other) const {
        if (this->size() != other.size()) return false; // si tamaño distinto, no pueden ser iguales

        // Creamos dos vectores con los IDs (unsigned) de los literales
        std::vector<unsigned> a;
        std::vector<unsigned> b;
        a.reserve(_lits.size());                         // reservamos memoria para evitar reallocs
        b.reserve(other._lits.size());

        // Convertimos los literales a IDs en "a"
        for (Literal l : _lits) a.push_back(l.get());
        // Convertimos los literales del otro Clause a IDs en "b"
        for (Literal l : other._lits) b.push_back(l.get());

        // Ordenamos ambos vectores para comparar sin depender del orden original
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());

        // Si los IDs ordenados coinciden, entonces las cláusulas tienen los mismos literales
        return a == b;
    }

} // namespace sat
