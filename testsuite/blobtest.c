#include <stdio.h>

typedef double number;

number num1 = 1.234;
number num2 = 5.678;
number num4[4] = { 7.1, 7.2, 7.3, 7.4 };

number sum12, sum13, sum14;

float fnum = 3.14;
double dnum;

void do_blobtest() {
    int i = 3;
    dnum = fnum;
    number num3 = 1.337;
    sum12 = num1+num2;
    sum13 = num1+num3;
    sum14 = num1+num4[i];
    /*
    if (num1 < num2) {
        printf("correct\n");
    } else {
        printf("incorrect\n");
    }
    if (num1 > num2) {
        printf("incorrect\n");
    } else {
        printf("correct\n");
    }
    */

}

int main() {
    do_blobtest();
    /*printf("dnum=%f sum12=%f  sum13=%f\n",*/
            /*dnum, sum12, sum13);*/
}

