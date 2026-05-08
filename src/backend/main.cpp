#include <iostream>
#include <vector>
#include <string>

#include "parser.cpp"

int main() {
    // A proper multi-line test program 
    std::string sourceCode = 
        "read x;\n"
        "if 0 < x then\n"
        "  fact := 1;\n"
        "  repeat\n"
        "    fact := fact * x;\n"
        "    x := x - 1;\n"
        "  until x = 0;\n"
        "  write \"Factorial is \", fact\n;"
        "else\n"
        "  write \"Input must be a positive integer.\";\n"
        "end;";

    std::cout << "--- TINY COMPILER: LEXER PHASE ---\n";
    
    Scanner scanner(sourceCode);
    std::vector<Token> tokens = scanner.scanAll(); // Using your awesome helper method!

    std::cout << "Lexing complete. Found " << tokens.size() << " tokens.\n\n";

    std::cout << "--- TINY COMPILER: PARSER PHASE ---\n";
    
    // 1. Pass the tokens into the new Parser
    Parser parser(tokens);
    parser.useTerminators = true; // Enable C#/C++ style terminators (semicolon)

    // 2. Build the Abstract Syntax Tree
    TreeNode* astRoot = parser.parse();

    // 3. Print the Tree visually to the console
    if (astRoot != nullptr) {
        std::cout << "\nAbstract Syntax Tree Successfully Built:\n\n";
        parser.printTree(astRoot, 0); 
    } else {
        std::cout << "Failed to build the AST.\n";
    }

    std::cin.get();
    return 0;
}