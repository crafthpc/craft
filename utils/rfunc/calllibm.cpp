#include <math.h>
#include <iostream>

using namespace std;

double val = 1.2;
double sinval = 0.0;

int main()
{
    sinval = sin(val);
    cout << "sin(" << val << ") = " << sinval << endl;
    return 0;
}

