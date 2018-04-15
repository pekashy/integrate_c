#define PI 3.14159265359
#define JMAX 22
#define EPS 1.0e-4
#define JMAXP JMAX+1
#define K 5
#include <stdio.h>
#include <math.h>
double qsimp(double (*func)(double), double a, double b);
double qtrap(double (*func)(double), double a, double b);
