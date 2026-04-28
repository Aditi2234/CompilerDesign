#ifndef AST_H
#define AST_H
extern int semantic_error;
extern int is_void_main;
void reset_symbol_table();
typedef struct Node {
    char* type;
    char* value;
    struct Node* left;
    struct Node* right;
    struct Node** children;   
    int child_count;          
} Node;

Node* create_node(char* type, Node* left, Node* right);
Node* create_leaf(char* value);
Node* create_list(Node* first);
void print_ast_file(Node* root, FILE* f);
void generate_3ac(Node* root);
void semantic_check(Node* root);

#endif
