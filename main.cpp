#include <cstdio>

struct myStruct {
    int x;
    int y;
    int z;
};

int main() {
    printf("Hello World!\n");

    int x = 0;
    x = x + 1;

    myStruct s = {0, 1, 2};
    printf("s.x = %d\n", s.x);

    // x = 1
    printf("x = %d\n", x);

    return 0;
}
