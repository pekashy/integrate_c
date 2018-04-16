#define PI 3.14159265359
#define NEPS 6
#define JMAX 20
#define EPS 1*pow(10, -NEPS)
#define JMAXP JMAX+1
#define K 5
#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
double qsimp(double (*func)(double), double a, double b);
double qtrap(double (*func)(double), double a, double b);
