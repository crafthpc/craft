#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FMT "%14.10f "

int main() {

    float fax, fay, faz;
    float fbt;
    float fcx, fcy, fcz;
    double dax, day, daz;
    double dbt;
    double dcx, dcy, dcz;
    int i;

    fax = 1.0;
    fay = (sqrt(5.0)-1.0)/2.0;
    fbt = 1.0;
    fcx = 1.0;
    fcy = -1.0/3.0;

    dax = 1.0;
    day = (sqrt(5.0)-1.0)/2.0;
    dbt = 1.0;
    dcx = 1.0;
    dcy = -1.0/3.0;

    for (i=1; i<=40; i++) {

        faz = fax;
        fax = fay;
        fay = faz-fay;
        fbt = fbt * (sqrt(5.0)-1.0)/2.0;
        fcz = fcx;
        fcx = fcy;
        fcy = (fcx+fcz)/6.0;

        daz = dax;
        dax = day;
        day = daz-day;
        dbt = dbt * (sqrt(5.0)-1.0)/2.0;
        dcz = dcx;
        dcx = dcy;
        dcy = (dcx+dcz)/6.0;

        printf("phi ^ %2d = " FMT FMT FMT FMT FMT FMT"\n",
                i, fax, fbt, fcx, dax, dbt, dcx);
    }

    return EXIT_SUCCESS;
}

