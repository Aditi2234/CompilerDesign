#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

int temp_count = 1;
int label_count = 1;

char* generate(Node* node);

char* label_stack[100];
int top = -1;

void push_label(char* l) {
    label_stack[++top] = l;
}

char* pop_label() {
    return label_stack[top--];
}
char* new_temp() {
    char* t = malloc(10);
    sprintf(t, "t%d", temp_count++);
    return t;
}
char* new_label() {
    char* l = malloc(10);
    sprintf(l, "L%d", label_count++);
    return l;
}
void generate_condition(Node* node, char* Ltrue, char* Lfalse) {

    if (strcmp(node->type, "&&") == 0) {
        char* Lmid = new_label();
        generate_condition(node->left, Lmid, Lfalse);
        printf("%s:\n", Lmid);
        generate_condition(node->right, Ltrue, Lfalse);
    }
    else if (strcmp(node->type, "||") == 0) {
        char* Lmid = new_label();

        generate_condition(node->left, Ltrue, Lmid);

        printf("%s:\n", Lmid);
        generate_condition(node->right, Ltrue, Lfalse);
    }
    else {
        char* t = generate(node);

        printf("if %s goto %s\n", t, Ltrue);
        printf("goto %s\n", Lfalse);
    }
}

char* generate(Node* node) {
    if (!node) return NULL;
    if (node->value != NULL) {
        return node->value;
    }

    if (node->type && strcmp(node->type, ";") == 0) {
        if (node->left)
            generate(node->left);
        if (node->right)
            generate(node->right);
        return NULL;
    }

    // Declaration with assignment
    if (strcmp(node->type, "DECL_ASSIGN") == 0) {
        Node* rhs = node->right;
        if (rhs && rhs->type && rhs->value == NULL &&
            rhs->left && rhs->left->value != NULL &&
            rhs->right && rhs->right->value != NULL) {
            printf("%s = %s %s %s\n", node->left->value,
                   rhs->left->value, rhs->type, rhs->right->value);
            return node->left->value;
        }
        char* r = generate(node->right);
        printf("%s = %s\n", node->left->value, r);
        return node->left->value;
    }

    // Assignment
    if (strcmp(node->type, "=") == 0) {
        Node* rhs = node->right;
        if (rhs && rhs->type && rhs->value == NULL &&
            rhs->left && rhs->left->value != NULL &&
            rhs->right && rhs->right->value != NULL) {
            printf("%s = %s %s %s\n", node->left->value,
                   rhs->left->value, rhs->type, rhs->right->value);
            return node->left->value;
        }
        char* r = generate(node->right);
        printf("%s = %s\n", node->left->value, r);
        return node->left->value;
    }
    // PRINT handling 
    if (node->type && strcmp(node->type, "PRINT") == 0) {
        Node* list = node->left;
        if (list->child_count == 1) {
            char* val = list->children[0]->value;
            if (val[0] == '"') 
            {
                if (strstr(val, "%d") == NULL && strstr(val, "%f") == NULL &&
                    strstr(val, "%s") == NULL && strstr(val, "%c") == NULL) {
                    char cleaned[512];
                    int ci = 0;
                    int vlen = strlen(val);
                    for (int j = 0; j < vlen; j++) {
                        if (val[j] == '\\' && j+1 < vlen && val[j+1] == 'n') {
                            j++; continue;
                        }
                        cleaned[ci++] = val[j];
                    }
                    cleaned[ci] = '\0';

                    if (strcmp(cleaned, "\"\"") == 0) {
                        printf("call print\n");
                    } else {
                        char* t = new_temp();
                        printf("%s = %s\n", t, cleaned);
                        printf("param %s\n", t);
                        printf("call print\n");
                    }
                    return NULL;
                }
            } 
            else {
                printf("param %s\n", val);
                printf("call print\n");
                return NULL;
            }
        }

        if (list->child_count >= 1 && list->children[0]->value[0]=='"') {
            char* fmt_raw = list->children[0]->value;
            int flen = strlen(fmt_raw);

            char fmt_inner[512];
            strncpy(fmt_inner, fmt_raw + 1, flen - 2);
            fmt_inner[flen - 2]='\0';


            char* nl;
            while ((nl = strstr(fmt_inner, "\\n")) != NULL) {
                memmove(nl, nl + 2, strlen(nl + 2) + 1);
            }
            int var_idx = 1;
            char* p = fmt_inner;
            while (*p) {
                if (*p == '%' && (*(p+1) == 'd' || *(p+1) == 'f' ||
                                  *(p+1) == 's' || *(p+1) == 'c')) {
                    if (var_idx < list->child_count) {
                        char* val = generate(list->children[var_idx]);
                        printf("param %s\n", val);
                        var_idx++;
                    }
                    p += 2;

                    while (*p == ' ') p++;
                } else {
                    char seg[256];
                    int si = 0;
                    while (*p && !(*p == '%' && (*(p+1) == 'd' || *(p+1) == 'f' ||
                                                 *(p+1) == 's' || *(p+1) == 'c'))) {
                        seg[si++] = *p;
                        p++;
                    }
                    seg[si] = '\0';

                    while (si > 0 && seg[si-1] == ' ') {
                        seg[--si] = '\0';
                    }

                    if (strlen(seg) > 0) {
                        char* t = new_temp();
                        printf("%s = \"%s\"\n", t, seg);
                        printf("param %s\n", t);
                    }
                }
            }

            printf("call print\n");
            return NULL;
        }

        return NULL;
    }
    // SCAN 
    if (node->type && strcmp(node->type, "SCAN") == 0) {
        Node* list = node->left;

        // Get format string (first child)
        char* fmt = NULL;
        if (list->child_count > 0 && list->children[0]->value && list->children[0]->value[0] == '"') {
            fmt = list->children[0]->value;
        }

        int var_idx = 1;
        if (fmt) {
            char* p = fmt;
            while (*p) {
                if (*p == '%' && (*(p+1) == 'd' || *(p+1) == 'f' || *(p+1) == 's' || *(p+1) == 'c')) {
                    if (var_idx < list->child_count) {
                        Node* child = list->children[var_idx];
                        char* val = NULL;

                        // Handle ADDR node (&var)
                        if (child->type && strcmp(child->type, "ADDR") == 0 && child->left) {
                            val = child->left->value;
                        } else if (child->value) {
                            val = child->value;
                        }

                        if (val) {
                            if (*(p+1) == 'f') {
                                printf("read_float %s\n", val);
                            } else if (*(p+1) == 's') {
                                printf("read_str %s\n", val);
                            } else if (*(p+1) == 'c') {
                                printf("read_char %s\n", val);
                            } else {
                                printf("read_int %s\n", val);
                            }
                        }
                        var_idx++;
                    }
                    p += 2;
                } else {
                    p++;
                }
            }
        } else {
            // No format string fallback
            for (int i = 0; i < list->child_count; i++) {
                Node* child = list->children[i];
                char* val = NULL;
                if (child->type && strcmp(child->type, "ADDR") == 0 && child->left) {
                    val = child->left->value;
                } else if (child->value) {
                    val = child->value;
                }
                if (val && val[0] != '"') {
                    printf("read_int %s\n", val);
                }
            }
        }

        return NULL;
    }
    // IF 
    if (strcmp(node->type, "IF") == 0) {
        if (node->right == NULL)
            return NULL;

        char* L1 = new_label(); 
        char* L2 = new_label(); 

        printf("\n");
        generate_condition(node->left, L1, L2);

        printf("\n%s:\n", L1);
        generate(node->right);

        printf("\n%s:\n", L2);

        return NULL;
    }

    // IF-ELSE
    if (strcmp(node->type, "IF_ELSE") == 0) {
        Node* ifPart = node->left;

        char* L1 = new_label(); 
        char* L2 = new_label(); 
        char* L3 = new_label(); 

        printf("\n");

        generate_condition(ifPart->left, L1, L2);

        printf("\n%s:\n", L1);
        generate(ifPart->right);
        printf("goto %s\n", L3);

        printf("\n%s:\n", L2);
        generate(node->right);
        printf("goto %s\n", L3);   

        printf("\n%s:\n", L3);

        return NULL;
    }

    // ELIF 
    if (strcmp(node->type, "ELIF") == 0) {
        Node* ifPart = node->left;   

        char* L1 = new_label(); 
        char* L2 = new_label(); 

        printf("elif_marker\n");

        generate_condition(ifPart->left, L1, L2);

        printf("\n%s:\n", L1);
        generate(ifPart->right);

        if (node->right) {
            char* L3 = new_label();
            printf("goto %s\n", L3);
            printf("\n%s:\n", L2);
            generate(node->right);
            printf("goto %s\n", L3);
            printf("\n%s:\n", L3);
        } else {
            printf("\n%s:\n", L2);
        }
        return NULL;
    }

    // BLOCK wrapper 
    if (strcmp(node->type, "BLOCK") == 0) {
        generate(node->left);
        return NULL;
    }

    if (
        strcmp(node->type, "+") == 0 ||
        strcmp(node->type, "-") == 0 ||
        strcmp(node->type, "*") == 0 ||
        strcmp(node->type, "/") == 0 ||
        strcmp(node->type, "%") == 0 ||
        strcmp(node->type, "<") == 0 ||
        strcmp(node->type, ">") == 0 ||
        strcmp(node->type, "<=") == 0 ||
        strcmp(node->type, ">=") == 0 ||
        strcmp(node->type, "==") == 0 ||
        strcmp(node->type, "!=") == 0
    ) {
        char* l = generate(node->left);
        char* r = generate(node->right);

        char* t = new_temp();
        printf("%s = %s %s %s\n", t, l, node->type, r);
        return t;
    }
    // WHILE LOOP
    if (strcmp(node->type, "WHILE") == 0) {

        char* Lstart = new_label(); 
        char* Ltrue  = new_label();  
        char* Lfalse = new_label();  

        printf("%s:\n", Lstart);

        generate_condition(node->left, Ltrue, Lfalse);

        printf("\n%s:\n", Ltrue);
        generate(node->right);

        printf("goto %s\n", Lstart);
        printf("\n%s:\n", Lfalse);

        return NULL;
    }

    // FOR LOOP
    if (strcmp(node->type, "FOR") == 0) {
        Node* init = node->left;
        Node* cond = node->right->left;
        Node* body = node->right->right->left;
        Node* update = node->right->right->right;

        char* Lstart = new_label();
        char* Ltrue  = new_label();
        char* Lfalse = new_label();

        printf("for_init ");
        generate(init);

        printf("%s:\n", Lstart);
        generate_condition(cond, Ltrue, Lfalse);

        printf("\n%s:\n", Ltrue);
        generate(body);

        printf("for_update ");
        generate(update);

        printf("goto %s\n", Lstart);
        printf("\n%s:\n", Lfalse);

        return NULL;
    }

    // WHILE_TRUE 
    if (strcmp(node->type, "WHILE_TRUE") == 0) {
        char* Lstart = new_label();
        char* Ltrue  = new_label();
        char* Lfalse = new_label();

        printf("%s:\n", Lstart);
        printf("if 1 goto %s\n", Ltrue);
        printf("goto %s\n", Lfalse);

        printf("\n%s:\n", Ltrue);
        generate(node->left);

        printf("goto %s\n", Lstart);
        printf("\n%s:\n", Lfalse);

        return NULL;
    }

    // DO-WHILE LOOP
    if (strcmp(node->type, "DO_WHILE") == 0) {
        char* Lstart = new_label();
        char* Ltrue  = new_label();
        char* Lfalse = new_label();

        printf("do_while_start\n");
        printf("%s:\n", Lstart);
        generate(node->left);  

        generate_condition(node->right, Lstart, Lfalse);

        printf("\n%s:\n", Lfalse);
        printf("do_while_end\n");

        return NULL;
    }
    // RETURN
    if (strcmp(node->type, "RETURN") == 0) {
        char* val = generate(node->left);
        if (val && strcmp(val, "0") != 0)
            printf("return %s\n", val);
        return NULL;
    }
    return NULL;
}

void generate_3ac(Node* root) 
{
    generate(root);
}
