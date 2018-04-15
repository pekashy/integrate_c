#include <pthread.h>
#include <stdlib.h>
#include "count.h"

#define PI 3.14159265359

typedef struct borders{
    double a;
    double b;
} borders;

double f(double x){
    return x*x; //1- 0.041667 2- 0.291667
}

void* threadFunc(void* b){
    double (*fp) (double x)=f;
    borders* bord = (borders*) b;
    double delta=EPS;
    double s=bord->a, fin=s+delta, FSimp=0;
    while(fin<bord->b){
        FSimp+=qsimp(fp, s, fin);
        s+=delta;
        fin+=delta;
    }
    printf("%f %f %f\n", bord->a, bord->b, FSimp);
    double * ret= malloc(sizeof(double));
    *ret=FSimp;
    return ret;
}

int main() {
    int n=4;
    double a=0;
    double b=100;
    borders* bo=malloc(sizeof(borders)*n);
    for(int i=0; i<n; i++){
        bo[i].a=(b-a)/n*i;
        bo[i].b=(b-a)/n*(i+1);
    }
    double result=0;
    pthread_t threads[n];
    for(int i=0; i<n-1; i++){
        if(pthread_create(&threads[i], NULL, threadFunc, &bo[i])!=0){
            printf("err creating thread");
            return 0;
        }
    }
    double* ret[n];
    result=result+*((double*) threadFunc(&bo[n-1]));
    for(int i=0; i<n-1; i++){
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    } /* Wait until thread is finished */

    printf("%e\n", result);
    return 0;
}