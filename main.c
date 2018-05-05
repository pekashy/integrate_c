#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <zconf.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <limits.h>
#define EPS 1.e-4

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

void* threadFunc(void* b){
    printf("ID: %lu, CPU: %d\n", pthread_self(), sched_getcpu());
    sched_yield();
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
    int procNum=get_nprocs();
    int coreIdMax = -1;
    const char cheatcode[] = "fgrep -e 'processor' -e 'core id' /proc/cpuinfo";
    FILE *cpuinfo_file = popen(cheatcode, "r");
    FILE *crs = popen("grep 'core id' /proc/cpuinfo | grep -Eo '[0-9]{1,4}' | sort -rn | head -n 1",
                      "r"); //getting maximum cpuId
    if (!cpuinfo_file || !crs) {
        perror("error opening cpuinfo file");
        return NULL;
    }
    fscanf(crs, "%d", &coreIdMax);
    if (coreIdMax < 0) {
        printf("core num parsing error");
        return NULL;
    }
    pthread_t thre[procNum];
    pthread_t threads[n];
    core *cores = malloc(sizeof(core) * (coreIdMax + 1));
    proc *procs = malloc(sizeof(proc) * procNum);

    cpu* top = malloc(sizeof(cpu));
    int processor, coreId;
    int res = -1;
    borders *bo = malloc(sizeof(borders) * n);
    int loadCpu=  ( ((n / procNum) > (1)) ? (n / procNum) : (1) ); //per cpu
    int loadCore=2 * loadCpu;
    int bounded=0;
    pthread_attr_t attr;
    cpu_set_t mask;
    pthread_attr_init(&attr);
    int mCpu, mCore;
    int i=0;
    int k=n%procNum*(n>procNum);
    CPU_ZERO (&mask);
    CPU_SET ((mCpu=sched_getcpu()), &mask);
    pthread_setaffinity_np (pthread_self(), sizeof (cpu_set_t), &mask);
    procs[mCpu].load++;
    bounded=1;
    char c[33];
    sprintf(c, "head -%d /proc/cpuinfo | tail -1\0", (mCpu*27+12));
    FILE* cc=popen(c, "r");
    printf("%s\n", c);

    fscanf(cc, "core id         : %d", &mCore);
    cores[mCore].load++;
    printf("bounding %d %d\n", mCpu, mCore);
    while ((res = fscanf(cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
        procs[processor].aff = coreId;
        procs[processor].id = processor;
        while (procs[processor].load < loadCpu && cores[coreId].load < loadCore) {
            printf("processor : %d\ncore id : %d\n", processor, coreId);
            if (i < n - 1) {
                bo[i].a = a + (b - a) / n * i;
                bo[i].b = a + (b - a) / n * (i + 1);
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                i++;
            } else {
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&thre[i - n], &attr, threadFunc, &bo[0])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
            }
            procs[processor].load++;
            cores[coreId].load++;
        }
        if (k > 0) {
            if (i < n - 1) {
                bo[i].a = a + (b - a) / n * i;
                bo[i].b = a + (b - a) / n * (i + 1);
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                i++;
            } else {
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&thre[i - n], &attr, threadFunc, &bo[0])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
            }
            procs[processor].load++;
            //cores[coreId].load++;
            k--;
        }
    }

    if (res != EOF) {
        perror("fscanf #1");
        return NULL;
    }
    errno = 0;
    bo[0].a = a + (b - a) / n * 0;
    bo[0].b = a + (b - a) / n * (0 + 1);

    result=result+*((double*) threadFunc(&bo[n-1]));
    double* ret[n];
    /*for(int i=n-2; i>=0 && n>1; i--){
        if(!threads[i]) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    }  /*Wait until thread is finished */
    printf("%e\n", result);
    return 0;
}

