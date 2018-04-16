#include <pthread.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <zconf.h>
#include "count.h"

#define PI 3.14159265359

typedef struct borders{
    double a;
    double b;
    int n;
    double* summ;
} borders;

double f(double x){
    return x*x; //1- 0.041667 2- 0.291667
}

void* threadFunc(void* b){
    double (*fp) (double x)=f;
    //cpu_set_t cpuset;
    /*CPU_ZERO (&cpuset);
    CPU_SET (cores[n + 1], &cpuset);

    sched_setaffinity (pthread_self(), sizeof (cpu_set_t), &cpuset);
    */
    borders* bord = (borders*) b;
    double FSimp=qsimp(fp, bord->a, bord->b);
    printf("%f %f %f\n", bord->a, bord->b, FSimp);
    //double * ret= malloc(sizeof(double));
    *bord->summ=FSimp;
    return bord->summ;
}

int input(int argc, char** argv){
    char *endptr, *str;
    long val;
    if (argc < 2) {
        fprintf(stderr, "Thread num not stated, interpreting as 1 \n");
        return 1;
    }
    str = argv[1];
    errno = 0;    /* To distinguish success/failure after call */
    val = strtol(str, &endptr, 10);
    /* Check for various possible errors */
    if ((errno == ERANGE && (val == INT_MAX || val == INT_MIN))
        || (errno != 0 && val == 0)) {
        perror("range");
        return 0;
    }
    if (endptr == str) {
        fprintf(stderr, "Thread num not stated, interpreting as 1 \n");
        return 1;
    }
    /* If we got here, strtol() successfully parsed a number */
    return (int) val;
}

int main(int argc, char* argv[]) {
    int n=input(argc, argv);
    if(!n) return -1;
    double a=0;
    double b=100;
    borders* bo=malloc(sizeof(borders)*n);
    for(int i=0; i<n; i++){
        bo[i].a=(b-a)/n*i;
        bo[i].b=(b-a)/n*(i+1);
        bo[i].summ=malloc(sizeof(double));
        *bo[i].summ=0;
        bo[i].n=n;
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
    for(int i=0; i<n-1; i++){
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    } /* Wait until thread is finished */
    result=result+*((double*) threadFunc(&bo[n-1]));

    printf("%e\n", result);
    return 0;
}