#include <iostream>
#include <fstream>
#include <string>

#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "typechecker.h"
#include "x64/x64gen.hpp"

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        exit(1);
    }

    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + '\n';
    }

    file.close();
    return content;
}

void writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not write to file " << filename << std::endl;
        exit(1);
    }

    file << content;
    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: poloc <input_file>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = inputFile.substr(0, inputFile.find_last_of('.')) + ".s";

    std::string source = readFile(inputFile);

    Lexer lexer(source);

    Parser parser(lexer);
    std::shared_ptr<ProgramNode> program = parser.parseProgram();
    if (has_err) return 1;
    TypeChecker typeChecker;
    typeChecker.checkProgram(program);
    if (has_err) return 1;
    WatGen gen;
    gen.gen(program);
    if (has_err) return 1;
    std::string asmCode = gen.get_output();

    writeFile(outputFile, asmCode);

    //std::cout << "Compilation successful!" << std::endl;
    //std::cout << "Output file: " << outputFile << std::endl;

    return 0;
}
