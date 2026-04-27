#include <stdio.h>

int main() {
    int a = 5, b = 10, c = 15;
    int i = 0;

    // if - else if - else
    if (a > b) {
        printf("a is greater\n");
    } 
    else if (b > c) {
        printf("b is greater\n");
    } 
    else {
        printf("c is greatest\n");
    }

    // for loop
    for (int j = 0; j < 3; j++) {
        printf("for loop: %d\n", j);
    }

    // while loop
    while (i < 3) {
        printf("while loop: %d\n", i);
        i++;
    }

    return 0;
}