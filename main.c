#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
//#include <zconf.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define EPS 1.e-7
#define FUNC x*x

typedef struct borders{
    double a;
    double b;
} borders;

typedef struct core{
    int id;
    int n;
    int loadCore;
    int trashLoadCore;
    cpu_set_t mask;
    int load;
} core;

void* threadFunc(void* b){

    borders* bord = (borders*) b;
    double * summ;
    double ftrp=0;
    summ= malloc((sizeof(double)));
    double a= bord->a;
    long long n=(long long) ((bord->b-a)/EPS);
    long long count=0;
    for(double x=a+EPS; count<n; count=count+1){
        ftrp+=FUNC*(EPS);
        x+=EPS;
    }
    printf("proc %lu; proc %d; a %f\n", pthread_self(), sched_getcpu(), a);
    *summ=ftrp;
    return summ;
}

void* burst(void* b){
    for(;;);
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
    int procNum = get_nprocs();
    int coreIdMax = -1;
    const char cheatcode[] = "fgrep -e 'processor' -e 'core id' /proc/cpuinfo";
    FILE *cpuinfo_file = popen(cheatcode, "r");
    FILE *crs = popen("grep 'core id' /proc/cpuinfo | grep -Eo '[0-9]{1,4}' | sort -rn | head -n 1",
                      "r"); //getting maximum cpuId    //sched_setscheduler(pthread_self(), SCHED_FIFO, NULL);
    if (!cpuinfo_file || !crs) {
        perror("error opening cpuinfo file");
        return NULL;
    }
    fscanf(crs, "%d", &coreIdMax);
    if (coreIdMax < 0) {
        printf("core num parsing error");
        return NULL;
    }
    core *cpu = calloc(coreIdMax + 1, sizeof(core));
    for (int y = 0; y < coreIdMax + 1; y++) {
        cpu[y].id = -1;
        CPU_ZERO(&cpu[y].mask);
        cpu[y].load = 0;
    }
    int n = input(argc, argv);
    if(n>procNum) n=procNum;
    if (!n || n < 1) return -1;
    double a = 0;
    double b = 100;
    double result = 0;
    pthread_t thre[procNum + 1];
    pthread_t threads[n + 1];
    int processor, coreId;
    int res = -1;
    borders *bo = calloc(n, sizeof(borders));
    borders *bb = calloc(abs(n - procNum) + 1, sizeof(borders));
    printf("%d", abs((n - procNum)));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
   // *//bo[0].a = a;
   // bo[0].b = a + (b - a) / n;
    int coreNum = 0;
    while ((res = fscanf(cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
        if (cpu[coreId].id == -1) coreNum++;
        cpu[coreId].id = processor;
        CPU_SET(processor, &cpu[coreId].mask);
    }
    if (res != EOF) {
        perror("fscanf #1");
        return NULL;
    }
    int minLoadCore = procNum / coreNum;
    int k = n % coreNum;
    int r = k;
    int i=0, t = 0;
    for (int w = 0; w <= coreIdMax; w++) {
        if (cpu[w].id == -1) continue;
        cpu[w].loadCore = n / coreNum + 1 * (r > 0);
        cpu[w].trashLoadCore = (minLoadCore - cpu[w].loadCore) * ((minLoadCore - cpu[w].loadCore) > 0);
        r--;
        for (int tr = 0; tr < cpu[w].trashLoadCore+cpu[w].trashLoadCore*(cpu[w].loadCore==0 && w!=coreIdMax); tr++) {
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu[w].mask);
            bb[t].a = a;
            bb[t].b = b*100.0;

            if ((pthread_create(&thre[t], &attr, threadFunc, &bb[t])) != 0) {
                printf("err creating thread %d", errno);
                return 0;
            }
            //printf("starting %dth trash process #%lu on core %d | current load %d loadCore %d\n",
           //        t, thre[t], w, cpu[w].load, cpu[w].loadCore);
            t++;
            //cpu[w].load++;
        }

    }
    for(int w=0; w<=coreIdMax; w++) {
        if (cpu[w].id == -1) continue;
        while (cpu[w].load < cpu[w].loadCore) {
            bo[i].a = a + (b - a) / n * i;
            bo[i].b = a + (b - a) / n * (i + 1);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu[w].mask);
            if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                printf("err creating thread %d", errno);
                return 0;
            }
           //printf("starting %dth process #%lu on core %d | current load %d loadCore %d\n",
            //       i, threads[i], w, cpu[w].load,cpu[w].loadCore);
            //i, threads[i], w, cpu[w].load,loadCore);
            i++;
            cpu[w].load++;
        }
    }
    double* ret[n+1];
    for(int ii=0; ii<n+1; ii++){
        ret[ii]=malloc(sizeof(double));
    }
    result=0;
    for(int i=n-1; i>=0; i--){
        if(!threads[i]) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
        free(ret[i]);
    }
    printf("%e\n", result);
    return 0;
}