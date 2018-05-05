#define _GNU_SOURCE
# define __GNUC_GNU_INLINE__ 1
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <zconf.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <limits.h>
#define EPS 1.e-5
#define FUNC 1/(x*x+25)
typedef struct borders{
    double a;
    double b;
    int mCpu;
} borders;

typedef struct processor {
    int load;
} proc;

typedef struct core {
    int load;
} core;

double f(double x){
    return x;
}

void* threadFunc(void* b){
    //printf("ID: %lu, CPU: %d\n", pthread_self(), sched_getcpu());
    borders* bord = (borders*) b;
    /*if(bord->mCpu==-1){
        //sched_yield();
        pthread_setschedparam(pthread_self(), SCHED_OTHER, NULL);
    }
    else
        pthread_setschedparam(pthread_self(), SCHED_RR, NULL);*/
    double * summ;
    summ= malloc((sizeof(double)));
    double ftrp=0;
    summ= malloc((sizeof(double)));
    int n=(int) ((bord->b-bord->a)/EPS);
    int count=0;
    for(double x=bord->a+EPS; count<n; count++){
        ftrp+=FUNC*(EPS);
        x+=EPS;
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
    clock_t start = clock() ;

    int n=input(argc, argv);
    if(!n || n<1) return -1;
    double a=0;
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

    int processor, coreId;
    int res = -1;
    borders *bo = malloc(sizeof(borders) * n);
    borders *bb = malloc(sizeof(borders) * abs((n-procNum)));

    int loadCpu=  ( ((n / procNum) > (1)) ? (n / procNum) : (1) ); //per cpu
    int loadCore=2 * loadCpu;
    int bounded=0;
    pthread_attr_t attr;
    cpu_set_t mask;
    pthread_attr_init(&attr);
    int mCpu, mCore;
    int i=0;
    int k=n%procNum*(n>procNum);
    int t=0;
    bo[0].a = a;
    bo[0].b = a + (b - a) / n ;

    CPU_ZERO (&mask);
    CPU_SET ((mCpu=sched_getcpu()), &mask);
    pthread_setaffinity_np (pthread_self(), sizeof (cpu_set_t), &mask);
    procs[mCpu].load++;
    char c[33];
    sprintf(c, "head -%d /proc/cpuinfo | tail -1\0", (mCpu*27+12));
    FILE* cc=popen(c, "r");
   // printf("%s\n", c);

    fscanf(cc, "core id         : %d", &mCore);
    cores[mCore].load++;
    //printf("bounding %d %d\n", mCpu, mCore);
    sched_setscheduler(pthread_self(), SCHED_FIFO, NULL);
    clock_t end = clock() ;
    double elapsed_time = (end-start)/(double)CLOCKS_PER_SEC ;
    printf("PARSING DONE: %f\n", elapsed_time);
    while ((res = fscanf(cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
        while (procs[processor].load < loadCpu && cores[coreId].load < loadCore) {
            //printf("processor : %d\ncore id : %d\n", processor, coreId);
            end = clock() ;
            printf("CYCLE: %f\n", (end-start)/(double)CLOCKS_PER_SEC);

            if (i < n - 1) {
                bo[i].a = a + (b - a) / n * i;
                bo[i].b = a + (b - a) / n * (i + 1);
                bo[i].mCpu=mCpu;
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                i++;
            } else {
                bb[t].a=bo[0].a;
                bb[t].b=bo[0].b*3;
                bb[t].mCpu=mCpu;

                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&thre[i - n], &attr, threadFunc, &bb[t])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                t++;
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
                bb[t].a=bo[0].a;
                bb[t].b=bo[0].b*3;
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&thre[i - n], &attr, threadFunc, &bb[t])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                t++;
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
    bo[i].a = a + (b - a) / n * i;
    bo[i].b = a + (b - a) / n * (i+1);
    bo[i].mCpu=mCpu;
    //pthread_setschedparam(pthread_self(), SCHED_RR, NULL);
    end = clock() ;
    printf("STARTING MAIN: %f\n", (end-start)/(double)CLOCKS_PER_SEC);

    result=result+*((double*) threadFunc(&bo[n-1]));
    double end1=end;
    end = clock() ;
    printf("MAIN ENDED: %f\n COUNT TIME: %f\n ", (end-start)/(double)CLOCKS_PER_SEC, (end-end1)/(double)CLOCKS_PER_SEC);


    double* ret[n];
    for(int i=n-2; i>=0 && n>1; i--){
        if(!threads[i]) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    }  /*Wait until thread is finished */
    end = clock() ;
    printf("FINISH: %f\n", (end-start)/(double)CLOCKS_PER_SEC);

    printf("%e\n", result);
    return 0;
}

