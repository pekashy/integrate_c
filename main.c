#include "count.h"

typedef struct borders{
    double a;
    double b;
} borders;

typedef struct processor {
    int id;
    int aff;
    int load;
} proc;

typedef struct core {
    proc *procs[2];
    int load;
} core;

typedef struct cpu{
    core* cores;
    proc* procs;
} cpu;

double f(double x){
    return 1/(x*x+25);
}

cpu* getCpuTopology2(int* coreIdMax, int procsNum) {
    *coreIdMax = -1;
    const char cheatcode[] = "fgrep -e 'processor' -e 'core id' /proc/cpuinfo";
    FILE *cpuinfo_file = popen(cheatcode, "r");
    FILE *crs = popen("grep 'core id' /proc/cpuinfo | grep -Eo '[0-9]{1,4}' | sort -rn | head -n 1",
                      "r"); //getting maximum cpuId
    if (!cpuinfo_file || !crs) {
        perror("error opening cpuinfo file");
        return NULL;
    }
    fscanf(crs, "%d", coreIdMax);
    if (*coreIdMax < 0) {
        printf("core num parsing error");
        return NULL;
    }

    core *cores = malloc(sizeof(core) * (*coreIdMax + 1));
    proc *procs = malloc(sizeof(proc) * procsNum);

    cpu* top = malloc(sizeof(cpu));
    top->procs=procs;
    top->cores=cores;

    for (int g = 0; g <= *coreIdMax; g++) {
        cores[g].procs[0] = NULL;
        cores[g].procs[1] = NULL;
        cores[g].load = 0;
    }
    for (int g = 0; g < procsNum; g++) {//can be removed assuming we have hyperthr
        procs[g].id = g;
        procs[g].load = 0;
        procs[g].aff = -1;
    }
    int processor, coreId;
    int res = -1;
    while ((res = fscanf(cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
        procs[processor].aff = coreId;
        procs[processor].id = processor;
        if (!cores[coreId].procs[0]) //bounding
            cores[coreId].procs[0] = &procs[processor];
        else
            cores[coreId].procs[1] = &procs[processor];
    }
    if (res != EOF) {
        perror("fscanf #1");
        return NULL;
    }
    errno = 0;
    fclose(cpuinfo_file);
    fclose(crs);
    if (errno) {
        perror("closing error");
        return NULL;
    }
    return top;
}

int getCpu(cpu* top, int maxCoreId, int n, int procNum){
    int c=0, p=0;
    for(int i=0; i<=maxCoreId; i++) {
        if(!top->cores[i].procs[0]) continue; //no core with this id
        if (top->cores[i].load < (2 * n / procNum)) {
            c = i;
            break;
        }
    }
    if(top->cores[c].procs[0]->load<=top->cores[c].procs[1]->load) p=top->cores[c].procs[0]->id;
    else p=top->cores[c].procs[1]->id;
    top->cores[c].load++;
    top->procs[p].load++;
    return p;
}

void* threadFunc(void* b){
    double (*fp) (double x)=f;
    borders* bord = (borders*) b;
    double * summ;
    summ= malloc((sizeof(double)));
    double ftrp=0;
    int count=0;
    for(double i=bord->a+EPS; i<bord->b; i+=EPS){
        ftrp+=0.5*(fp(i-EPS)+fp(i))*(EPS);
        count++;
    }
    *summ=ftrp;
    return summ;
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
    if(!n || n<1) return -1;
    double a=1;
    double b=1000;
    double result = 0;
    pthread_t threads[n];
    int ncores=0;
    borders *bo = malloc(sizeof(borders) * n);
    pthread_attr_t attr;
    cpu_set_t mask;
    pthread_attr_init(&attr);
    int nprocs = get_nprocs();
    cpu *top = getCpuTopology2(&ncores, nprocs);
    if (!top) {
        printf("cpu topology parsing error");
        return 0;
    }

    pthread_t thre[nprocs];
    bo[n-1].a=a+(b-a)/n*(n-1);
    bo[n-1].b=a+(b-a)/n*(n);
    borders *bb = malloc(sizeof(borders));
    bb[0].a=a;
    bb[0].b=a+(b-a)/(n*0.3);

    CPU_ZERO (&mask);
    CPU_SET (sched_getcpu(), &mask);
    pthread_setaffinity_np (pthread_self(), sizeof (cpu_set_t), &mask);
    int mCpu = sched_getcpu();
    top->procs[mCpu].load++;
    top->cores[top->procs[mCpu].aff].load++;
    int proc;
    for (int i = 0; n > 1 && i < n - 1; i++) {
        if (mCpu != sched_getcpu()) {
            top->procs[mCpu].load--;
            top->cores[top->procs[mCpu].aff].load--;
            mCpu = sched_getcpu();
            top->procs[mCpu].load++;
            top->cores[top->procs[mCpu].aff].load++;
        }
        proc = getCpu(top, ncores, n, nprocs);
        bo[i].a = a + (b - a) / n * i;
        bo[i].b = a + (b - a) / n * (i + 1);

        CPU_ZERO(&mask);
        CPU_SET(proc, &mask);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
        if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
            printf("err creating thread");
            return 0;
        }
    }


    if(n<nprocs){
        for(int i=n; i<nprocs; i++){
            proc = getCpu(top, ncores, nprocs, nprocs);
            CPU_ZERO(&mask);
            CPU_SET(proc, &mask);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
            if ((pthread_create(&thre[i-n], &attr, threadFunc, &bb[0])) != 0) {
                printf("err creating thread");
                return 0;
            }
        }
    }
    result=result+*((double*) threadFunc(&bo[n-1]));
    double* ret[n];
    for(int i=n-2; i>=0 && n>1; i--){
        if(!threads[i]) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    }  /*Wait until thread is finished */
    printf("%e\n", result);
    return 0;
}