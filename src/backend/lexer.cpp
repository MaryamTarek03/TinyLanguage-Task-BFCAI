#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_map>
#include "token.h"

#ifdef _WIN32
    #define API_EXPORT __declspec(dllexport)
#else
    #define API_EXPORT __attribute__((visibility("default")))
#endif

class Scanner {
private:
    std::string source;
    int pos;
    int line;
    std::unordered_map<std::string, TokenType> reservedWords;

    bool isEOF() {
        return pos >= source.length();
    }

    // Helper: look at the current character
    char peek() {
        if (isEOF()) return '\0';
        return source[pos];
    }

    // Helper: read the current character and move forward
    char advance() {
        if (isEOF()) return '\0';
        return source[pos++];
    }

    // Helper: skip whitespaces and comments 
    void skipWhitespaceAndComments() {
        while (!isEOF()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r') {
                advance();
            } else if (c == '\n') {
                line++;
                advance();
            } else if (c == '{') { // Comments cannot be nested
                while (!isEOF() && peek() != '}') {
                    if (peek() == '\n') line++;
                    advance();
                }
                advance(); // read '}'
            } else {
                break;
            }
        }
    }

public:
    Scanner(const std::string& input) : source(input), pos(0), line(1) {
        reservedWords["if"] = IF_KW;
        reservedWords["then"] = THEN_KW;
        reservedWords["else"] = ELSE_KW;
        reservedWords["end"] = END_KW;
        reservedWords["repeat"] = REPEAT_KW;
        reservedWords["until"] = UNTIL_KW;
        reservedWords["read"] = READ_KW;
        reservedWords["write"] = WRITE_KW;
    }

    Token getNextToken() {
        skipWhitespaceAndComments();

        if (isEOF()) return {ENDFILE, "EOF", line, pos, 0};

        // Record the exact starting position, 
        // which is useful for error reporting and frontend highlighting
        int startPos = pos; 
        char c = peek();

        // Identifiers and Reserved Words
        if (isalpha(c)) {
            std::string lexeme = "";
            while (isalnum(peek())) {
                lexeme += advance();
            }
            if (reservedWords.find(lexeme) != reservedWords.end()) {
                return {reservedWords[lexeme], lexeme, line, startPos, (int)lexeme.length()};
            }
            return {ID, lexeme, line, startPos, (int)lexeme.length()};
        }

        // Numbers
        if (isdigit(c)) {
            std::string lexeme = "";
            while (isdigit(peek())) {
                lexeme += advance();
            }
            
            // // If the number is immediately followed by a letter, it's a lexical error (e.g., "6f")
            // if (isalpha(peek())) {
            //     while (isalnum(peek())) {
            //         lexeme += advance();
            //     }
            //     return {UNKNOWN, lexeme, line, startPos, (int)lexeme.length()};
            // }
            
            return {NUMBER, lexeme, line, startPos, (int)lexeme.length()};
        }

        // Strings
        if (c == '"') {
            std::string lexeme = "";
            advance(); // Read the opening quote
            while (peek() != '"' && !isEOF()) {
                if (peek() == '\n') line++; // Allow multi-line strings
                lexeme += advance();
            }
            if (isEOF()) { // Uncomplete string
                // Maybe return an error token instead of UNKNOWN
                return {UNKNOWN, lexeme, line, startPos, (int)lexeme.length() + 1};
            }
            advance(); // Read the closing quote
            return {STRING, lexeme, line, startPos, (int)lexeme.length() + 2};
        }

        // Symbols and Operators
        advance(); 
        std::string lexeme(1, c);

        switch (c) {
            case '+': return {ADDOP, lexeme, line, startPos, 1};
            case '-': return {SUBOP, lexeme, line, startPos, 1};
            case '*': return {MULOP, lexeme, line, startPos, 1};
            case '/': return {DIVOP, lexeme, line, startPos, 1};
            case '=': return {COMPARISONOP, lexeme, line, startPos, 1};
            case '<': return {COMPARISONOP, lexeme, line, startPos, 1};
            case ';': return {SEMICOLON, lexeme, line, startPos, 1};
            case ',': return {COMMA, lexeme, line, startPos, 1};
            case '(': case ')': return {PUNCTUATION, lexeme, line, startPos, 1};
            case ':':
                if (peek() == '=') {
                    lexeme += advance();
                    return {ASSIGNMENTOP, lexeme, line, startPos, 2}; // := is 2 chars
                }
                return {UNKNOWN, lexeme, line, startPos, 1};
            default:
                return {UNKNOWN, lexeme, line, startPos, 1};
        }
    }
    
    std::vector<Token> scanAll() {
        std::vector<Token> tokens;
        Token t;
        do {
            t = getNextToken();
            tokens.push_back(t);
        } while (t.type != ENDFILE);
        return tokens;
    }
};



// --------------------------------- C++/C Interop for C# ---------------------------------
// This part is for making the C++ code callable from C# by exporting a simple C-style API.


#include <cstring>
#include <cstdlib>

// Helps C# read the functions because it prevents the complex C++ name mangling (Gibberish function names in the compiled .so file)
// Like '_Z8TokenizePKc' instead of 'Tokenize'
// It basically tells the compiler "Don't mess with the function names, just export them as they are"
extern "C" {

    struct CToken {
        int type;
        const char* lexeme;
        int line;
        int startIndex;
        int length;
    };

    struct LexResult {
        CToken* tokens;
        int count;
    };

    // The function C# will call
    // We use __attribute__((visibility("default"))) to ensure Linux exports it correctly in the .so file
    API_EXPORT LexResult Tokenize(const char* sourceCode) {

        Scanner scanner(sourceCode);
        std::vector<Token> cppTokens = scanner.scanAll();

        LexResult result;
        result.count = cppTokens.size();
        result.tokens = new CToken[result.count];

        for (size_t i = 0; i < cppTokens.size(); i++) {
            result.tokens[i].type = cppTokens[i].type; // Enum implicitly casts to int
            result.tokens[i].line = cppTokens[i].line;
            
            // strdup allocates memory for the string so it doesn't vanish when this function ends
            result.tokens[i].lexeme = strdup(cppTokens[i].lexeme.c_str()); 
            result.tokens[i].startIndex = cppTokens[i].startIndex;
            result.tokens[i].length = cppTokens[i].length;
        }

        return result;
    }

    // The function C# will call to free memory
    API_EXPORT void FreeResult(LexResult result) {
        for (int i = 0; i < result.count; i++) {
            free((void*)result.tokens[i].lexeme); // Free the strings
        }
        delete[] result.tokens; // Free the array_Z8TokenizePKc
    }
}