#include "count.h"
#include <stdlib.h>



double trapzd(double (*func)(double), double a, double b, int n) {
    double x,tnm,sum,del;
    static double s;
    int it,j;

    if (n == 1) {
        return (s=0.5*(b-a)*(func(a)+func(b)));
    } else {
        for (it=1,j=1;j<n-1;j++) it <<= 1;
        tnm=it;
        del=(b-a)/tnm;
        x=a+0.5*del;
        for (sum=0.0,j=1;j<=it;j++,x+=del) sum += func(x);
        s=0.5*(s+(b-a)*sum/tnm);
        return s;
    }
}

void polint(double xa[], double ya[], int n, double x, double *y, double *dy) {
    int i,m,ns=1;
    double den,dif,dift,ho,hp,w;
    double c[n];
    double d[n];

    dif=fabs(x-xa[0]);
    //c=malloc(n*sizeof(char));
    //d=malloc(n*sizeof(char));
    for (i=0;i<n;i++) {
        if ( (dift=fabs(x-xa[i])) < dif) {
            ns=i;
            dif=dift;
        }
        c[i]=ya[i];
        d[i]=ya[i];
    }
    *y=ya[ns--];
    for (m=0;m<n;m++) {
        for (i=0;i<n-m;i++) {
            ho=xa[i]-x;
            hp=xa[i+m]-x;
            w=c[i+1]-d[i];
            if ( (den=ho-hp) == 0.0) return;
            den=w/den;
            d[i]=hp*den;
            c[i]=ho*den;
        }
        *y += (*dy=(2*ns < (n-m) ? c[ns+1] : d[ns--]));
    }
}



double qsimp(double (*func)(double), double a, double b) {
    int j;
    double s,st,ost=0.0,os=0.0;
    //printf("LOL %.3e %.3e\n",a,b);
    for (j=1;j<=JMAX;j++) {
        st=trapzd(func,a,b,j);
        //printf("%.3e\n",st);
        s=(4.0*st-ost)/3.0;
        if (j > 5)
            if (fabs(s-os) < EPS*fabs(os) ||
                (s == 0.0 && os == 0.0)){
                //printf("qsimp took %d steps to reach eps\n", j);
                return s;
            }
        os=s;
        ost=st;
    }
    return 0.0;
}

double qtrap(double (*func)(double), double a, double b) {
    int j;
    double s,olds=0.0;

    for (j=1;j<=JMAX;j++) {
        s=trapzd(func,a,b,j);
        if (j > 5)
            if (fabs(s-olds) < EPS*fabs(olds) || (s == 0.0 && olds == 0.0)) {
                //printf("qtrap took %d steps to reach eps\n", j);
                return s;
            }
        olds=s;
    }
    return 0.0;
}


double qromb(double (*func)(double), double a, double b) {
    double ss,dss;
    double s[JMAXP],h[JMAXP+1];
    int j;

    h[0]=1.0;
    for (j=0;j<JMAX;j++) {
        s[j]=trapzd(func,a,b,j);
        if (j >= K) {
            polint(&h[j-K],&s[j-K],K,0.0,&ss,&dss);
            if (fabs(dss) <= EPS*fabs(ss)){
                //printf("qromb took %d steps to reach eps\n", j);
                return ss;
            }
        }
        h[j+1]=0.25*h[j];
    }
    return 0.0;
}

