#include "count.h"

double trapzd(double (*func)(double), double a, double b, int n, double s) {
    double x,tnm,sum,del;
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
        //printf("         %d: TRAPZD: s: %f it: %d del:%f\n",pthread_self(), s, it, del);
        return s;
    }
}

double qsimp(double (*func)(double), double a, double b) {
    int j;
    double s,st,ost,os;
    ost=os=0;
    //printf("LOL %.3e %.3e\n",a,b);
    for (j=1;j<=JMAX;j++) {
        st=trapzd(func,a,b,j, s);
        //printf("%d: %.3e %.3e\n",pthread_self(), st, ost);
        s=(4.0*st-ost)/3.0;
        //printf("%d: CHECK %d s: %f os: %f | %f %f\n", pthread_self(), j, s, os, fabs(s - os), EPS*fabs(os));
        if (j > 5) {
            if (fabs(s - os) < EPS*fabs(os) ||
                (s == 0.0 && os == 0.0)) {
                //printf("qsimp took %d steps to reach eps\n", j);
                return s;
            }
        }
        os=s;
        ost=st;

    }
    printf("---%d: EPSILON %f %f %e\n",pthread_self(), s, os, fabs(s-os));
    return s;
}