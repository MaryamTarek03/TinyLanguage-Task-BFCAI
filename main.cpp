#include <iostream>
#include "lexer.h"

int main() {
    // Example
    std::string sourceCode =
        "if x = 10 then\n"
        "  x := x + 1;\n"
        "end\n"
        "{ this is a comment and should be ignored }\n";

    Scanner scanner(sourceCode);
    std::vector<Token> tokens = scanner.scanAll();

    std::cout << "Line\tToken Type\tLexeme\n";
    std::cout << "--------------------------------------\n";
    for (const auto &token : tokens)
    {
        if (token.type == ENDFILE)
            break;
        std::cout << token.line << "\t"
                  << token.toString() << "\t\t"
                  << token.lexeme << "\n";
    }

    std::cin.get();
    return 0;
}