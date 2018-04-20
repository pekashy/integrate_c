#include "count.h"

typedef struct borders{
    double a;
    double b;
    int n;
    double* summ;
} borders;

int* cores;

double f(double x){
    return x*x; //1- 0.041667 2- 0.291667
}

int* getCpuTopology() {
    char cpuinfo[] = "lscpu -p=cpu,core";
    FILE* f = popen (cpuinfo, "r");
    if (!f){
        perror ("error opening cpuinfo file");
        return NULL;
    }
    int nproc = sysconf (_SC_NPROCESSORS_CONF);
    int out[nproc*2];
    int* cores = (int*) malloc ((nproc + 1) * sizeof (int));
    if (!cores) {
        perror ("malloc error");
        return NULL;
    }
    for (int i = 0; i <= nproc; i++)
        cores[i] = -1;
    int coreNum = 0;
    fscanf(f, "%*[^\n]\n", NULL); //skiping # lines in output
    fscanf(f, "%*[^\n]\n", NULL);
    fscanf(f, "%*[^\n]\n", NULL);
    fscanf(f, "%*[^\n]\n", NULL);

    for(int i=0; i<nproc*2; i+=2){
        fscanf(f, "\n%d,%d", &out[i],&out[i+1]);
        printf("%d %d\n", out[i], out[i+1]);
    }
    /*if (res == EOF) {
        perror ("cpuinfo parsing error");
        return NULL;
    }*/
    //i=0;
    for(int a=0; a<nproc; a++){
        if (cores[out[a*2]] == -1) {
            cores[out[a*2]] = out[a*2+1];
            coreNum++;
        }
    }
    errno = 0;
    fclose (f);
    if (errno) {
        perror ("closing error");
        return NULL;
    }
    return cores;
}

void* threadFunc(void* b){
    double (*fp) (double x)=f;
    borders* bord = (borders*) b;
    cpu_set_t mask;
    CPU_ZERO (&mask);
    CPU_SET (cores[bord->n], &mask);
    printf("trying to bound thread %d to core %d of cpusetsize %d: %d (0 is success)\n",bord->n, cores[bord->n], sizeof (cpu_set_t), pthread_setaffinity_np (pthread_self(), sizeof (cpu_set_t), &mask));
    double FSimp=qsimp(fp, bord->a, bord->b);
    //printf("%f %f %f\n", bord->a, bord->b, FSimp);
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
    cores=getCpuTopology();
    for(int i=0; i<n; i++){
        bo[i].a=(b-a)/n*i;
        bo[i].b=(b-a)/n*(i+1);
        bo[i].summ=malloc(sizeof(double));
        *bo[i].summ=0;
        bo[i].n=i;
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