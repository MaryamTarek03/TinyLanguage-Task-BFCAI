#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_map>
#include "token.h"

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

        if (isEOF()) return {ENDFILE, "EOF", line};

        char c = peek();

        // Identifiers and Reserved Words
        if (isalpha(c)) {
            std::string lexeme = "";
            while (isalnum(peek())) {
                lexeme += advance();
            }
            if (reservedWords.find(lexeme) != reservedWords.end()) {
                return {reservedWords[lexeme], lexeme, line};
            }
            return {ID, lexeme, line};
        }

        // Numbers
        if (isdigit(c)) {
            std::string lexeme = "";
            while (isdigit(peek())) {
                lexeme += advance();
            }
            return {NUMBER, lexeme, line};
        }

        // Symbols and Operators
        advance(); // read the character
        std::string lexeme(1, c);

        switch (c) {
            case '+': return {ADDOP, lexeme, line};
            case '-': return {SUBOP, lexeme, line};
            case '*': return {MULOP, lexeme, line};
            case '/': return {DIVOP, lexeme, line};
            case '=': return {COMPARISONOP, lexeme, line};
            case '<': return {COMPARISONOP, lexeme, line};
            case ';': return {SEMICOLON, lexeme, line};
            case ',': return {COMMA, lexeme, line};
            case '(': case ')': return {PUNCTUATION, lexeme, line};
            case ':':
                if (peek() == '=') {
                    lexeme += advance();
                    return {ASSIGNMENTOP, lexeme, line}; // := operator
                }
                return {UNKNOWN, lexeme, line};
            default:
                return {UNKNOWN, lexeme, line};
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
