#include <iostream>      // std::cout, std::cerr
#include <fstream>       // std::ifstream, std::ofstream
#include <filesystem>    // std::filesystem::create_directories, path, stem
#include <chrono>        // medir tiempo
#include <iomanip>       // std::setprecision, std::fixed
#include <sstream>       // std::ostringstream  (te faltaba para durationStr)

#include "Solver/heuristics.hpp"   // sat::Solver
#include "Solver/Solver.hpp"   // sat::Solver
#include "Solver/inout.hpp"    // lectura/escritura DIMACS

std::string formatDuration(double durationSec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    if (durationSec < 1.0) {
        oss << durationSec * 1000.0 << " ms";
    } else if (durationSec < 60.0) {
        oss << durationSec << " s";
    } else {
        oss << (durationSec / 60.0) << " min";
    }

    return oss.str();
}

int main(int argc, char **argv) {
    using namespace sat; // para escribir Solver, Clause, inout::...

    // argc = número de argumentos en la línea de comandos
    // argv[0] = nombre del ejecutable
    // argv[1] = archivo .cnf (lo que necesitamos)
    if (argc < 2) {
        std::cout << "c Usage: " << argv[0] << " <instance.cnf>\n"; // "c" = comentario estilo DIMACS
        return 1; // error: faltan argumentos
    }


    // 2) Abrir fichero CNF
    std::ifstream ifs(argv[1]); // abrir archivo CNF para lectura
    if (!ifs.is_open()) {       // si falló abrirlo...
        std::cout << "c Could not open file " << argv[1] << "\n";
        return 1; // error
    }

  
    // 3) Leer DIMACS (parseo)
    
    // rawClauses: vector de cláusulas, cada cláusula = vector<Literal>
    // numVars: número de variables del problema
    auto [rawClauses, numVars] = inout::read_from_dimacs(ifs);

    
    // 4) Crear solver y cargar cláusulas-
    // Creamos el solver con numVars variables (0..numVars-1)
    Solver solver(static_cast<unsigned>(numVars));
    Solver s2(static_cast<unsigned>(numVars), FirstVariable {});
    Solver s3(numVars, RandomVariable{});
    Solver s4(numVars, RandomVariable{});
    Solver s5(numVars, RandomVariable{});
    // Añadimos una a una todas las cláusulas al solver
    for (auto &vec : rawClauses) {
        // std::move(vec) mueve el vector (no copia) -> más eficiente
        // vec queda "vacío" después (pero eso da igual, ya no lo usamos)
        solver.addClause(Clause(std::move(vec)));
        s2.addClause(Clause(std::move(vec)));
        s3.addClause(Clause(std::move(vec)));
        s4.addClause(Clause(std::move(vec)));
        s5.addClause(Clause(std::move(vec)));
    }

    
    // 5) Medir tiempo SOLO de solve()   
    auto start = std::chrono::high_resolution_clock::now(); // inicio reloj alta resolución
    const bool isSat = solver.solve_with_restart(); // aquí corre el algoritmo SAT (DPLL + propagación)
    auto end = std::chrono::high_resolution_clock::now();   // fin
    auto durationSec = std::chrono::duration<double>(end - start).count(); // duración en segundos (double)

    start = std::chrono::high_resolution_clock::now(); // inicio reloj alta resolución
    const bool isSat2 = s2.solve_with_restart(); // aquí corre el algoritmo SAT (DPLL + propagación)
    end = std::chrono::high_resolution_clock::now();   // fin
    auto durationSec2 = std::chrono::duration<double>(end - start).count(); // duración en segundos (double)

    start = std::chrono::high_resolution_clock::now();
    const bool isSat3 = s3.solve_with_restart();
    end = std::chrono::high_resolution_clock::now();
    auto durationSec3 = std::chrono::duration<double>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    const bool isSat4 = s4.solve_with_restart();
    end = std::chrono::high_resolution_clock::now();
    auto durationSec4 = std::chrono::duration<double>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    const bool isSat5 = s5.solve_with_restart();
    end = std::chrono::high_resolution_clock::now();
    auto durationSec5 = std::chrono::duration<double>(end - start).count();

    // 6) Formatear tiempo para imprimir bonito   
    std::string t1 = formatDuration(durationSec);
    std::string t2 = formatDuration(durationSec2);
    std::string t3 = formatDuration(durationSec3);
    std::string t4 = formatDuration(durationSec4);
    std::string t5 = formatDuration(durationSec5);


    // 7) Crear carpeta de salida
    
    // Crea ../sol si no existe (si ya existe, no pasa nada)
    std::filesystem::create_directories("../sol/restart");

    
    // 8) Construir ruta del fichero de salida
    
    std::string inputFile(argv[1]); // ruta input como string
    // stem() = nombre del archivo sin extensión
    // ejemplo: uf20-0184.cnf -> uf20-0184
    std::string outputFile = "../sol/restart/" + std::filesystem::path(inputFile).stem().string() + ".cnf";

    
    // 9) Abrir fichero de salida
    
    std::ofstream ofs(outputFile); // abrir archivo para escritura
    if (!ofs.is_open()) {          // si falló...
        std::cerr << "c Could not open output file " << outputFile << "\n";
        return 1;
    }

    
    // 10) Escribir tiempo en el fichero de salida
    
    // Línea tipo DIMACS comentario (c ...)
    ofs << "c Execution time FirstVariable: " << t2 << "\n";
    ofs << "c Execution time MostConstrained: " << t1 << "\n";
    ofs << "c Execution time RandomVariable1: " << t3 << "\n";
    ofs << "c Execution time RandomVariable2: " << t4 << "\n";
    ofs << "c Execution time RandomVariable3: " << t5 << "\n";
    
    // 11) Caso UNSAT
    
    if (!isSat) {
        // Guardar UNSAT en el fichero
        ofs << "UNSAT\n";

        // Mostrar en consola también
        std::cout << "UNSAT (saved to " << outputFile
                  << ", time: " << t1 << ")\n";

        if (!isSat2 || !isSat3 || !isSat4 || !isSat5){
            std::cout << "UNSAT (saved to " << outputFile
                      << ", time2: " << t2
                      << ", time3: " << t3
                      << ", time4: " << t4
                      << ", time5: " << t5 << ")\n";
        }
        return 0; // terminamos bien (simplemente no hay solución)
    }

    
    // 12) Caso SAT: guardar solución
    
    // solver.getUnitLiterals() devuelve el "trail" final:
    // todos los literales asignados (decisiones + propagaciones)
    // inout::to_dimacs(...) lo convierte a formato DIMACS (enteros +/- y 0 final)
    ofs << inout::to_dimacs(solver.getUnitLiterals());

    // Mensaje por consola
    std::cout << "SAT (solution saved to " << outputFile
            << ", time FirstVariable: " << t2
            << ", time MostConstrained: " << t1 
            << ", RandomVariable1: " << t3
            << ", RandomVariable2: " << t4
            << ", RandomVariable3: " << t5
            << ")\n";


    return 0; // OK
}
