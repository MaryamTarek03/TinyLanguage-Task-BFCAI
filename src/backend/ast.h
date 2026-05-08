#pragma once

#include <string>
#include "token.h"

const int MAX_CHILDREN = 3;

// 1. Broad categories of nodes
enum NodeKind { StatementNode, ExpressionNode };

// 2. Specific types of Statements
enum StmtKind { IfK, RepeatK, AssignK, ReadK, WriteK };

// 3. Specific types of Expressions (Math & Variables)
enum ExpKind { OpK, ConstK, IdK, StringK };

struct TreeNode {
    // Up to 3 children (e.g., IF statement has: 1. Condition, 2. Then body, 3. Else body)
    TreeNode* children[MAX_CHILDREN];
    
    // The next line of code to execute in this block
    TreeNode* sibling;
    
    // What kind of node is this?
    NodeKind nodeKind;
    StmtKind stmtKind;
    ExpKind expKind;
    
    // Payload Data
    TokenType op;           // For operators like ADDOP, COMPARISONOP
    int val;                // For NUMBER constants
    std::string name;       // For ID variables
    int line;               // For error reporting
    std::string string_val; // For STRING literals
    
    // Initialize pointers to null
    TreeNode() {
        for (int i = 0; i < MAX_CHILDREN; i++) children[i] = nullptr;
        sibling = nullptr;
    }
};