#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ast.h"

int semantic_error = 0;
Node* create_node(char* type, Node* left, Node* right) {
    Node* n=(Node*)malloc(sizeof(Node));
    n->type=strdup(type);
    n->value=NULL;
    n->left=left;
    n->right=right;
    return n;
}

Node* create_leaf(char* value) {
    Node* n=(Node*)malloc(sizeof(Node));
    n->type=NULL;
    n->value=strdup(value);
    n->left=n->right = NULL;
    return n;
}
Node* create_list(Node* first) {
    Node* n=malloc(sizeof(Node));
    n->type=strdup("LIST");
    n->children=malloc(sizeof(Node*) * 100);
    n->children[0]=first;
    n->child_count=1;
    return n;
}

void print_ast_file(Node* root, FILE* f) {
    if (!root)
        return;

    if (root->value!=NULL) {
        fprintf(f, "%s", root->value);
        return;
    }

    fprintf(f, "(%s ", root->type);
    print_ast_file(root->left, f);
    fprintf(f, " ");
    print_ast_file(root->right, f);
    fprintf(f, ")");
}

char* symbol_table[100];
int symbol_count=0;
char* reported_errors[100];
int error_count=0;
char* initialized_vars[100];
int init_count=0;
int return_count=0;

void reset_symbol_table() 
{
    symbol_count=0;
    error_count=0;
    init_count=0;
    return_count=0;
}

int is_initialized(char* var) {
    for (int i=0;i<init_count;i++) {
        if (strcmp(initialized_vars[i], var) == 0)
            return 1;
    }
    return 0;
}

void mark_initialized(char* var) {
    if (!is_initialized(var)) {
        initialized_vars[init_count++] = strdup(var);
    }
}

int exists(char* var) {
    for (int i=0;i<symbol_count; i++) {
        if (strcmp(symbol_table[i], var) == 0)
            return 1;
    }
    return 0;
}

void add_symbol(char* var) {
    if (!exists(var)) {
        symbol_table[symbol_count++]=strdup(var);  
    }
}

int error_reported(char* var) {
    for (int i =0;i<error_count; i++) {
        if (strcmp(reported_errors[i], var) == 0)
            return 1;
    }
    return 0;
}

void add_error(char* var) {
    if (!error_reported(var)) {
        reported_errors[error_count++] = strdup(var);  
    }
}

int is_number(char* str) {
    int i=0;
    int dot_seen=0;

    if (str[0]=='-') 
        i = 1;

    while (str[i]!='\0') {
        if (str[i]=='.') {
            if (dot_seen) 
                return 0;  
            dot_seen=1;
        }
        else if (str[i] < '0' || str[i] > '9') {
            return 0;  
        }
        i++;
    }

    return 1;  
}

void semantic_check(Node* node) {
    if (!node) return;

    // Sequence
    if (node->type && strcmp(node->type, ";") == 0) {
        semantic_check(node->left);
        semantic_check(node->right);
        return;
    }
    
    // IF
    if (node->type && strcmp(node->type, "IF") == 0) {
        semantic_check(node->left);  
        semantic_check(node->right);  
        return;
    }

    // IF-ELSE
    if (node->type && strcmp(node->type, "IF_ELSE") == 0) {
        semantic_check(node->left);   
        semantic_check(node->right);  
        return;
    }
    if (node->type && strcmp(node->type, "BLOCK") == 0) {
        semantic_check(node->left);
        return;
    }
    if (node->type && strcmp(node->type, "ELIF") == 0) {
        semantic_check(node->left);
        semantic_check(node->right);
        return;
    }
    if (node->type && strcmp(node->type, "WHILE") == 0) {
        semantic_check(node->left);
        semantic_check(node->right);
        return;
    }
    if (node->type && strcmp(node->type, "FOR") == 0) {
        semantic_check(node->left);  
        semantic_check(node->right);  
        return;
    }
    if (node->type && strcmp(node->type, "FOR_CTRL") == 0) {
        semantic_check(node->left);  
        semantic_check(node->right);  
        return;
    }
    if (node->type && strcmp(node->type, "FOR_BODY") == 0) {
        semantic_check(node->left);   
        semantic_check(node->right);  
        return;
    }
    if (node->type && strcmp(node->type, "WHILE_TRUE") == 0) {
        semantic_check(node->left);   
        return;
    }
    if (node->type && strcmp(node->type, "DO_WHILE") == 0) {
        semantic_check(node->left);  
        semantic_check(node->right); 
        return;
    }
    // RETURN 
    if (node->type && strcmp(node->type, "RETURN") == 0) {
        return_count++;
        if (return_count > 1) {
            printf("Warning: unreachable code\n");
        }
        if (is_void_main && node->left != NULL) {
            printf("Semantic Error: Cannot return a value from void function\n");
            semantic_error= 1;
        }
        if (node->left) semantic_check(node->left);
        return;
    }
    // DECL
    if (node->type && strcmp(node->type, "DECL") == 0) {
        char* var = node->left->value;

        if (exists(var)) {
            if (!error_reported(var)) {
                printf("Semantic Error: Variable '%s' already declared\n", var);
                add_error(var);
            }
            semantic_error = 1;
        } 
        else {
            add_symbol(var);
        }
        return;
    }

    //  DECL_ASSIGN
    if (node->type && strcmp(node->type, "DECL_ASSIGN") == 0) {
        char* var = node->left->value;

        if (exists(var)) {
            if (!error_reported(var)) {
                printf("Semantic Error: Variable '%s' already declared\n", var);
                add_error(var);
            }
            semantic_error = 1;
        } else {
            add_symbol(var);
            mark_initialized(var);
        }

        semantic_check(node->right);
        return;
    }

    // Assignment
    if (node->type && strcmp(node->type, "=") == 0) {
        char* var = node->left->value;

        if (!exists(var)) {
            if (!error_reported(var)) {
                printf("Semantic Error: Variable '%s' not declared\n", var);
                add_error(var);
            }
            semantic_error = 1;
        } else {
            mark_initialized(var);
        }

        semantic_check(node->right);
        return;
    }

    // Division by zero
    if (node->type && (strcmp(node->type, "/") == 0 || strcmp(node->type, "%") == 0)) {
        semantic_check(node->left);
        if (node->right && node->right->value && strcmp(node->right->value, "0") == 0) {
            printf("Semantic Error: Division by zero\n");
            semantic_error = 1;
        }
        semantic_check(node->right);
        return;
    }
    //PRINT
    if (node->type && strcmp(node->type, "PRINT") == 0) {
        Node* list = node->left;
        if (list && list->child_count > 0) {
            Node* first = list->children[0];
            if (first && first->value && first->value[0] != '"') {
                printf("Semantic Error: printf expects a format string, not a variable\n");
                semantic_error = 1;
                return;
            }

            if (first && first->value && first->value[0] == '"') {
                int spec_count = 0;
                char* p = first->value;
                while (*p) {
                    if (*p == '%' && (*(p+1) == 'd' || *(p+1) == 'f' ||
                                     *(p+1) == 's' || *(p+1) == 'c')) {
                        spec_count++;
                        p += 2;
                    } else {
                        p++;
                    }
                }
                int arg_count = list->child_count - 1;
                if (spec_count != arg_count) {
                    printf("Semantic Error: printf format expects %d arguments but %d provided\n",
                           spec_count, arg_count);
                    semantic_error = 1;
                    return;
                }
            }
        }
        for (int i = 0; i < list->child_count; i++) {
            semantic_check(list->children[i]);
        }
        return;
    }

    if (node->type && strcmp(node->type, "SCAN") == 0) {
        Node* list = node->left;
        if (list && list->child_count > 0) {
            // Check first argument is a format string
            Node* first = list->children[0];
            if (first && first->value && first->value[0] != '"') {
                printf("Semantic Error: scanf expects a format string as first argument\n");
                semantic_error = 1;
                return;
            }

            // Count format specifiers and validate argument count
            if (first && first->value && first->value[0] == '"') {
                int spec_count = 0;
                char* p = first->value;
                while (*p) {
                    if (*p == '%' && (*(p+1) == 'd' || *(p+1) == 'f' ||
                                     *(p+1) == 's' || *(p+1) == 'c')) {
                        spec_count++;
                        p += 2;
                    } else {
                        p++;
                    }
                }
                int arg_count = list->child_count - 1;
                if (spec_count != arg_count) {
                    printf("Semantic Error: scanf format expects %d arguments but %d provided\n",
                           spec_count, arg_count);
                    semantic_error = 1;
                    return;
                }
            }

            // Check variables are declared and mark as initialized
            for (int i = 1; i < list->child_count; i++) {
                Node* child = list->children[i];
                char* var = NULL;

                // Handle &var (ADDR node)
                if (child->type && strcmp(child->type, "ADDR") == 0 && child->left) {
                    var = child->left->value;
                }
                // Handle var without & 
                else if (child->value) {
                    var = child->value;
                }

                if (var && var[0] != '"') {
                    if (!exists(var)) {
                        if (!error_reported(var)) {
                            printf("Semantic Error: Variable '%s' not declared\n", var);
                            add_error(var);
                        }
                        semantic_error = 1;
                    } else {
                        mark_initialized(var);
                    }
                }
            }
        }
        return;
    }

    if (node->value != NULL) {
        if (!is_number(node->value) && node->value[0] != '"') {
            if (!exists(node->value)) {
                if (!error_reported(node->value)) {
                    printf("Semantic Error: Variable '%s' used before declaration\n", node->value);
                    add_error(node->value);
                }
                semantic_error = 1;
            } else if (!is_initialized(node->value)) {
                if (!error_reported(node->value)) {
                    printf("Semantic Error: Variable '%s' used without initialization\n", node->value);
                    add_error(node->value);
                }
                semantic_error = 1;
            }
        }
        return;
    }

    semantic_check(node->left);
    semantic_check(node->right);
}
