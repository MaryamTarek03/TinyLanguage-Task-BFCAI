#include <string>

enum TokenType {
    // Reserved Words
    IF_KW, THEN_KW, ELSE_KW, END_KW, REPEAT_KW, UNTIL_KW, READ_KW, WRITE_KW,
    // Other
    ID, NUMBER, STRING,
    // Operators
    ADDOP, SUBOP, MULOP, DIVOP,
    COMPARISONOP, ASSIGNMENTOP,
    // Symbols
    SEMICOLON, PUNCTUATION, COMMA,
    // System
    ENDFILE, UNKNOWN
};

// Helper to convert enum to string
std::string tokenToString(TokenType type) {
    switch (type) 
    {
        case IF_KW: return "IF_KW"; case THEN_KW: return "THEN_KW";
        case ELSE_KW: return "ELSE_KW"; case END_KW: return "END_KW";
        case REPEAT_KW: return "REPEAT_KW"; case UNTIL_KW: return "UNTIL_KW";
        case READ_KW: return "READ_KW"; case WRITE_KW: return "WRITE_KW";
        case ID: return "ID"; case NUMBER: return "NUMBER"; case STRING: return "STRING";
        case ADDOP: return "ADDOP"; case SUBOP: return "SUBOP";
        case MULOP: return "MULOP"; case DIVOP: return "DIVOP";
        case COMPARISONOP: return "COMPARISONOP"; case ASSIGNMENTOP: return "ASSIGNMENTOP";
        case SEMICOLON: return "SEMICOLON"; case PUNCTUATION: return "PUNCTUATION";
        case COMMA: return "COMMA"; case ENDFILE: return "ENDFILE";
        default: return "UNKNOWN";
    }
}


struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int startIndex;
    int length;

    std::string toString() const {
        return tokenToString(type);
    }
};