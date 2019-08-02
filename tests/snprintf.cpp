#include <stdio.h>

int main () {
    char a[16];
    size_t i;

    i = snprintf(a, 13, "%012d", 12345);  // 第 1 种情况
    printf("i = %lu, a = %s\n", i, a);    // 输出：i = 12, a = 000000012345

    i = snprintf(a, 9, "%012d", 12345);   // 第 2 种情况
    printf("i = %lu, a = %s\n", i, a);    // 输出：i = 12, a = 00000001

    int j = 10;
    snprintf(a, 13, "%02d", j);
    printf("i = %lu, a = %s\n", i, a);  

    return 0;

}
