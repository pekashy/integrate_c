#define PI 3.14159265359
#define EPS 1.0e-3
#define JMAX 22
#define JMAXP JMAX+1
#define K 5
#include <stdio.h>
#include <math.h>
double qsimp(double (*func)(double), double a, double b);
double qtrap(double (*func)(double), double a, double b);
double qromb(double (*func)(double), double a, double b);

