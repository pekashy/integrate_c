#include <pthread.h>
#include "count.h"

#define PI 3.14159265359

typedef struct borders{
    double a;
    double b;
} borders;

double f(double x){
    return x*x;
}

void* threadFunc(void* b){
    double (*fp) (double x)=f;
    borders* bord = (borders*) b;
    //FILE *out=fopen("grph.txt", "wb");
    double s=bord->a, fin=s+0.0001, FTrap, FSimp, FRomberg;
    FTrap=0; FSimp=0; FRomberg=0;
    //printf("%e %e %e\n",fin,bord->b, FRomberg);

    while(fin<bord->b){
        //FTrap+=qtrap(fp, s, fin);
        //FSimp+=qsimp(fp, s, fin);
        //printf("AAA %e\n",FRomberg);
        FRomberg+=qromb(fp, s, fin);
        //fprintf(out, "%e %e %e %e\n", (s+fin)/2, FTrap, FSimp, FRomberg);
        s+=0.0001;
        fin+=0.0001;
    }
    printf("%e",FRomberg);
}

int main() {
    int n=2;
    borders b={0, 1};
    pthread_t threads[n];
    int status=pthread_create(&threads[0], NULL, threadFunc, &b);
    return pthread_join(threads[0], NULL); /* Wait until thread is finished */

    return 0;
}




