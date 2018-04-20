#define _GNU_SOURCE

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <zconf.h>

#define PI 3.14159265359
#define NEPS 5
#define JMAX 40
#define EPS 1*pow(10, -NEPS)
#define JMAXP JMAX+1
#define K 5

double qsimp(double (*func)(double), double a, double b);
