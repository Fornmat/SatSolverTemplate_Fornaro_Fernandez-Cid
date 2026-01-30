/**
* @author Tim Luchterhand
* @date 26.11.24
* @brief Implementación de las estructuras básicas Variable y Literal
*/

#include "basic_structures.hpp"   // Incluye las declaraciones de Variable y Literal

namespace sat {                  // Espacio de nombres del solver SAT

    // -------------------------
    // Variable
    // -------------------------

    // Constructor de Variable: guarda el identificador de la variable
    Variable::Variable(unsigned val) : _val(val) {}

    // Devuelve el identificador interno de la variable
    unsigned Variable::get() const {
        return _val;
    }

    // Compara dos variables comprobando si tienen el mismo identificador
    bool Variable::operator==(Variable other) const {
        return _val == other._val;
    }

    // -------------------------
    // Literal
    // -------------------------

    // Constructor de Literal: guarda el identificador del literal
    Literal::Literal(unsigned val) : _val(val) {}

    // Devuelve el identificador interno del literal, PARES NEGATIVOS, IMPARES POSITIVOS
    unsigned Literal::get() const {
        return _val;
    }

    // Devuelve el literal negado, del lit negativo te devuelve el positivo y del lit positivo el negativo
    Literal Literal::negate() const {
        // Cambia el último bit:
        // par <-> impar  => negativo <-> positivo
        return Literal(_val ^ 1u);
    }

    // Devuelve el signo del literal
    short Literal::sign() const {
        // Identificador par  => literal negativo => -1  (si es par negativo si es impar positivo)
        // Identificador impar => literal positivo => +1
        return (_val % 2u == 0u) ? static_cast<short>(-1)
                                 : static_cast<short>(+1);
    }

    // Compara dos literales comprobando si tienen el mismo identificador
    bool Literal::operator==(Literal other) const {
        return _val == other._val;
    }

    // -------------------------
    // Helper functions
    // -------------------------

    // Crea el literal positivo de una variable
    Literal pos(Variable x) {
        // Identificador impar para el literal positivo, identificador de una variable x 2 + 1
        return Literal(2u * x.get() + 1u);
    }

    // Crea el literal negativo de una variable, identificador de una variable x 2 
    Literal neg(Variable x) {
        // Identificador par para el literal negativo
        return Literal(2u * x.get());
    }

    // Obtiene la variable asociada a un literal, 
    Variable var(Literal l) {
        // Dividir entre 2 elimina el bit de signo, se queda con el lower casse 
        return Variable(l.get() / 2u);
    }

} // namespace sat
