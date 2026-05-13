#include <vector>
#include <iostream>
#include <cstring>
#include "lexer.cpp"
#include "ast.h"

struct SyntaxError {
    int line;
    int charIndex;
    std::string message;
};

class Parser {
private:
    std::vector<Token> tokens;
    int tokenIndex;
    Token currentToken;

    bool panicMode = false;

    // Helper: Move to the next token
    void advance() {
        if (tokenIndex < tokens.size() - 1) {
            tokenIndex++;
            currentToken = tokens[tokenIndex];
        }
    }

    // Panic Mode Recovery: Skip tokens until we find a statement boundary
    void synchronize() {
        panicMode = false;
        while (currentToken.type != ENDFILE) {
            if (currentToken.type == SEMICOLON || 
                currentToken.type == END_KW || 
                currentToken.type == UNTIL_KW || 
                currentToken.type == ELSE_KW ||
                isStatementStart(currentToken.type)) {
                return; // Reached a safe starting point
            }
            advance();
        }
    }

    // Helper: Assert the current token is what we expect, then advance
    void match(TokenType expected) {
        if (currentToken.type == expected) {
            advance();
        } else {
            if (!panicMode) {
                std::string typeStr = (currentToken.type == UNKNOWN) ? "Lexical Error: " : "Syntax Error: ";
                std::string msg = typeStr + "Expected " + tokenToString(expected) + " but got '" + currentToken.lexeme + "'";
                errors.push_back({currentToken.line, currentToken.startIndex, msg});
                panicMode = true; // Enter panic mode
            }
            synchronize(); // Try to recover
        }
    }

    // Helper to check if a token can start a statement
    bool isStatementStart(TokenType type) {
        return type == IF_KW || 
               type == REPEAT_KW || 
               type == ID || 
               type == READ_KW || 
               type == WRITE_KW;
    }

    // --- Create Nodes ---
    TreeNode* newStmtNode(StmtKind kind) {
        TreeNode* t = new TreeNode();
        t->nodeKind = StatementNode;
        t->stmtKind = kind;
        t->line = currentToken.line;
        return t;
    }

    TreeNode* newExpNode(ExpKind kind) {
        TreeNode* t = new TreeNode();
        t->nodeKind = ExpressionNode;
        t->expKind = kind;
        t->line = currentToken.line;
        return t;
    }

    // 1. The Sequence Linker
    // TreeNode* stmt_sequence() {
    //     TreeNode* t = statement();
    //     TreeNode* p = t;
        
    //     // Loop as long as we see semicolons
    //     while (currentToken.type == SEMICOLON) {
    //         match(SEMICOLON);
    //         TreeNode* q = statement();
    //         if (q != nullptr) {
    //             if (t == nullptr) {
    //                 t = p = q;
    //             } else {
    //                 p->sibling = q; // Chain the next statement as a sibling!
    //                 p = q;
    //             }
    //         }
    //     }
    //     return t;
    // }
    TreeNode* stmt_sequence() {
        TreeNode* t = statement();
        
        // If we are in C# mode, EVERY statement must end with a semicolon immediately.
        if (useTerminators) {
            match(SEMICOLON); 
        }

        TreeNode* p = t;
        
        // THE LOOP DECISION:
        // Separator Mode: Keep looping as long as we see a ';'
        // Terminator Mode: Keep looping as long as we see a word that starts a statement
        while ((!useTerminators && currentToken.type == SEMICOLON) || 
               (useTerminators && isStatementStart(currentToken.type))) {
            
            // In Tiny Language mode, we eat the separator BEFORE parsing the next statement
            if (!useTerminators) {
                match(SEMICOLON);
            }
            
            
            // Re-arm panic mode status before processing next statement
            panicMode = false;
            
            TreeNode* q = statement();
            
            if (q != nullptr) {
                if (t == nullptr) t = p = q;
                else {
                    p->sibling = q;
                    p = q;
                }
            }
            
            // In C# mode, we eat the terminator AFTER parsing the next statement
            if (useTerminators) {
                match(SEMICOLON);
            }
        }
        return t;
    }
    // 2. The Statement Router
    TreeNode* statement() {
        TreeNode* t = nullptr;
        switch (currentToken.type) {
            case IF_KW:     t = if_stmt(); break;
            case REPEAT_KW: t = repeat_stmt(); break;
            case ID:        t = assign_stmt(); break;
            case READ_KW:   t = read_stmt(); break;
            case WRITE_KW:  t = write_stmt(); break;
            default: {
                if (!panicMode) {
                    std::string typeStr = (currentToken.type == UNKNOWN) ? "Lexical Error: " : "Syntax Error: ";
                    std::string msg = typeStr + "Unexpected token '" + currentToken.lexeme + "'";
                    errors.push_back({currentToken.line, currentToken.startIndex, msg});
                    panicMode = true;
                }
                advance(); // Skip the bad token
                synchronize(); // Try to recover
                break;
            }
        }
        return t;
    }

    // 3. The Assignment Statement
    TreeNode* assign_stmt() {
        TreeNode* t = newStmtNode(AssignK);
        
        // Grab the variable name
        t->name = currentToken.lexeme; 
        match(ID);
        
        // Match the :=
        match(ASSIGNMENTOP); 
        
        // The right side of the assignment is always an expression
        t->children[0] = exp(); 
        
        return t;
    }
    TreeNode* if_stmt() {
        TreeNode* t = newStmtNode(IfK);
        match(IF_KW);
        
        t->children[0] = exp(); // 1. Parse the condition
        
        match(THEN_KW);
        t->children[1] = stmt_sequence(); // 2. Parse the 'then' block
        
        // 3. Handle the OPTIONAL 'else' part
        if (currentToken.type == ELSE_KW) {
            match(ELSE_KW);
            t->children[2] = stmt_sequence();
        }
        
        match(END_KW); // Terminated with end
        return t;
    }

    TreeNode* repeat_stmt() {
        TreeNode* t = newStmtNode(RepeatK);
        match(REPEAT_KW);
        
        t->children[0] = stmt_sequence(); // 1. Parse the body first
        
        match(UNTIL_KW);
        t->children[1] = exp(); // 2. Parse the condition
        
        return t;
    }
    TreeNode* read_stmt() {
        TreeNode* t = newStmtNode(ReadK);
        match(READ_KW);
        
        // Parse the first variable
        TreeNode* firstId = newExpNode(IdK);
        firstId->name = currentToken.lexeme;
        match(ID); // It MUST be an ID
        
        t->children[0] = firstId;
        TreeNode* currentId = firstId;
        
        // If we see a comma, loop and attach the next variable as a sibling!
        while (currentToken.type == COMMA) {
            match(COMMA);
            
            TreeNode* nextId = newExpNode(IdK);
            nextId->name = currentToken.lexeme;
            match(ID);
            
            currentId->sibling = nextId; // Chain it!
            currentId = nextId;          // Move the pointer forward
        }
        
        return t;
    }
    TreeNode* write_stmt() {
        TreeNode* t = newStmtNode(WriteK);
        match(WRITE_KW);
        
        // Helper lambda to parse either a string or an expression
        auto parseWriteItem = [&]() -> TreeNode* {
            if (currentToken.type == STRING) { // Use whatever you named your token!
                TreeNode* strNode = newExpNode(StringK);
                strNode->string_val = currentToken.lexeme;
                match(STRING);
                return strNode;
            } else {
                return exp(); // Otherwise, parse a normal math/variable expression
            }
        };

        // Parse the first item
        t->children[0] = parseWriteItem();
        TreeNode* currentItem = t->children[0];
        
        // Loop for commas
        while (currentToken.type == COMMA) {
            match(COMMA);
            
            TreeNode* nextItem = parseWriteItem();
            currentItem->sibling = nextItem; // Chain it!
            currentItem = nextItem;
        }
        
        return t;
    }
    // 1. Comparisons (<, =)
    TreeNode* exp() {
        TreeNode* t = simple_exp();
        
        if (currentToken.type == COMPARISONOP) {
            TreeNode* p = newExpNode(OpK);
            p->children[0] = t;
            p->op = currentToken.type;
            p->name = currentToken.lexeme; // Store the '<' or '=' string
            
            match(COMPARISONOP);
            p->children[1] = simple_exp();
            t = p;
        }
        return t;
    }

    // 2. Addition and Subtraction (+, -)
    TreeNode* simple_exp() {
        TreeNode* t = term();
        
        while (currentToken.type == ADDOP || currentToken.type == SUBOP) {
            TreeNode* p = newExpNode(OpK);
            p->children[0] = t;
            p->op = currentToken.type;
            p->name = currentToken.lexeme; // Store the '+' or '-'
            
            match(currentToken.type);
            p->children[1] = term();
            t = p;
        }
        return t;
    }

    // 3. Multiplication and Division (*, /)
    TreeNode* term() {
        TreeNode* t = factor();
        
        while (currentToken.type == MULOP || currentToken.type == DIVOP) {
            TreeNode* p = newExpNode(OpK);
            p->children[0] = t;
            p->op = currentToken.type;
            p->name = currentToken.lexeme; // Store the '*' or '/'
            
            match(currentToken.type);
            p->children[1] = factor();
            t = p;
        }
        return t;
    }

    // 4. Base Values (Numbers, Variables, Parentheses)
    TreeNode* factor() {
        TreeNode* t = nullptr;
        
        if (currentToken.type == NUMBER) {
            t = newExpNode(ConstK);
            t->val = std::stoi(currentToken.lexeme); // Convert string to integer
            match(NUMBER);
        } 
        else if (currentToken.type == ID) {
            t = newExpNode(IdK);
            t->name = currentToken.lexeme;
            match(ID);
        } 
        else if (currentToken.type == PUNCTUATION && currentToken.lexeme == "(") {
            match(PUNCTUATION); // Match the '('
            t = exp();          // Evaluate the math inside the parentheses
            match(PUNCTUATION); // Match the ')'
        } 
        else {
            if (!panicMode) {
                std::string typeStr = (currentToken.type == UNKNOWN) ? "Lexical Error: " : "Syntax Error: ";
                std::string msg = typeStr + "Unexpected token in expression -> '" + currentToken.lexeme + "'";
                errors.push_back({currentToken.line, currentToken.startIndex, msg});
                panicMode = true;
            }
            advance();
            synchronize(); // Try to recover
        }
        
        return t;
    }

    public:
    // false = Tiny Language standard (Separators)
    // true  = C#/C++ standard (Terminators)
    bool useTerminators = false;
    std::vector<SyntaxError> errors;

    void printTree(TreeNode* tree, int indentLevel = 0) {
        while (tree != nullptr) {
            // Print indentation
            for (int i = 0; i < indentLevel; i++) std::cout << "  ";

            // Print the node details
            if (tree->nodeKind == StatementNode) {
                switch (tree->stmtKind) {
                    case IfK: std::cout << "If\n"; break;
                    case RepeatK: std::cout << "Repeat\n"; break;
                    case AssignK: std::cout << "Assign to: " << tree->name << "\n"; break;
                    case ReadK: std::cout << "Read: " << tree->name << "\n"; break;
                    case WriteK: std::cout << "Write\n"; break;
                    default: std::cout << "Unknown Statement Node\n"; break;
                }
            } else if (tree->nodeKind == ExpressionNode) {
                switch (tree->expKind) {
                    case OpK: std::cout << "Op: " << tree->name << "\n"; break;
                    case ConstK: std::cout << "Const: " << tree->val << "\n"; break;
                    case IdK: std::cout << "Id: " << tree->name << "\n"; break;
                    case StringK: std::cout << "String: \"" << tree->string_val << "\"\n"; break;
                    default: std::cout << "Unknown Expression Node\n"; break;
                }
            }

            // Recursively print children (with extra indentation)
            for (int i = 0; i < 3; i++) {
                if (tree->children[i] != nullptr) {
                    printTree(tree->children[i], indentLevel + 1);
                }
            }

            // Move to the next sibling (same indentation)
            tree = tree->sibling;
        }
    }
    // Constructor takes the output from your Lexer
    Parser(const std::vector<Token>& lexerOutput) {
        tokens = lexerOutput;
        tokenIndex = 0;
        if (!tokens.empty()) {
            currentToken = tokens[0];
        }
    }

    // The main entry point
    TreeNode* parse() {
        TreeNode* root = stmt_sequence();
        if (currentToken.type != ENDFILE) {
            std::string typeStr = (currentToken.type == UNKNOWN) ? "Lexical Error: " : "Syntax Error: ";
            std::string msg = typeStr + "Unexpected tokens at end of file: '" + currentToken.lexeme + "'";
            errors.push_back({currentToken.line, currentToken.startIndex, msg});
        }
        return root;
    }
};




#ifdef _WIN32
    #define API_EXPORT __declspec(dllexport)
#else
    #define API_EXPORT __attribute__((visibility("default")))
#endif


extern "C" {
    // 1. The C-friendly Error Struct
    struct CSyntaxError {
        int line;
        int charIndex;
        const char* message;
    };

    // 2. The Array Wrapper
    struct CParseResult {
        CSyntaxError* errors;
        int errorCount;
    };

    // 3. The Function .NET will call
    API_EXPORT CParseResult ParseCode(const char* sourceCode, bool useTerminators = true) {
        // Run the whole pipeline internally!
        Scanner scanner(sourceCode);
        std::vector<Token> tokens = scanner.scanAll();
        
        Parser parser(tokens);
        parser.useTerminators = useTerminators; // Set the mode based on the parameter
        parser.parse(); // This builds the AST and populates parser.errors

        CParseResult result;
        result.errorCount = parser.errors.size();

        if (result.errorCount > 0) {
            result.errors = new CSyntaxError[result.errorCount];
            for (int i = 0; i < result.errorCount; i++) {
                result.errors[i].line = parser.errors[i].line;
                result.errors[i].charIndex = parser.errors[i].charIndex;
                result.errors[i].message = strdup(parser.errors[i].message.c_str()); 
            }
        } else {
            result.errors = nullptr;
        }

        return result;
    }

    // 4. The Golden Rule: Free the memory!
    API_EXPORT void FreeParseResult(CParseResult result) {
        if (result.errors != nullptr) {
            for (int i = 0; i < result.errorCount; i++) {
                free((void*)result.errors[i].message); // Free the strdup
            }
            delete[] result.errors; // Free the array
        }
    }
}