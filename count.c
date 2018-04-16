#include "count.h"
#include <stdlib.h>
#include <pthread.h>


double trapzd(double (*func)(double), double a, double b, int n) {
    double x,tnm,sum,del;
    static double s;
    int it,j;

    if (n == 1) {
        return (s=0.5*(b-a)*(func(a)+func(b)));
    } else {
        it=(int) pow(2, n-1);
        tnm=it;
        del=(b-a)/tnm;
        x=a+0.5*del;
        for (sum=0.0,j=1;j<=it;j++,x+=del) sum += func(x);
        s=0.5*(s+(b-a)*sum/tnm);
        printf("TRAPZD: s: %f it: %d del:%f/n", s, it, del);
        return s;
    }
}

double qsimp(double (*func)(double), double a, double b) {
    int j;
    double s,st,ost=0.0,os=0.0;
    printf("LOL %.3e %.3e\n",a,b);
    for (j=1;j<=JMAX;j++) {
        st=trapzd(func,a,b,j);
        //printf("%.3e\n",st);
        s=(4.0*st-ost)/3.0;
        printf("%d: CHECK %d s: %f os: %f | %f %f\n", pthread_self(), j, s, os, fabs(s - os), EPS * fabs(os));
        if (j > 5) {
            if (fabs(s - os) <= EPS * fabs(os) ||
                (s == 0.0 && os == 0.0)) {
                printf("qsimp took %d steps to reach eps\n", j);
                return s;
            }
        }
        os=s;
        ost=st;

    }
    printf("\"%d: EPSILON %f %f %e\n",pthread_self(), s, os, fabs(s-os));
    return s;
}

double qtrap(double (*func)(double), double a, double b) {
    int j;
    double s,olds=0.0;

    for (j=1;j<=JMAX;j++) {
        s=trapzd(func,a,b,j);
        if (j > 5)
            if (fabs(s-olds) <= EPS*fabs(olds) || (s == 0.0 && olds == 0.0)) {
                printf("qtrap took %d steps to reach eps\n", j);
                return s;
            }
        olds=s;
    }
    return 0.0;
}
