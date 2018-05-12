#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <zconf.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define EPS 1.e-5
#define FUNC 1/(sin(x*x)+2)

typedef struct borders{
    double a;
    double b;
} borders;

typedef struct core{
    int id;
    int n;
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
    *summ=ftrp;
    return summ;
}

void* burst(void* b){
    borders* bord = (borders*) b;
    double * summ;
    double ftrp=0;
    double a= bord->a;
    long long n=(long long) ((bord->b-a)/EPS);
    long long count=0;
    double x=a+EPS;
    /*for(double x=a+EPS; count<n; count=count+1){
        //double*r =malloc(20000*sizeof(double));
        //memset(r, '1', 20000*sizeof(double));
        //free(r);

                //if(!count%15000) sleep(1000);
        count % ((int) pow(x, 2343*sin(x*x)))*count % ((int) pow(x, 2343*sin(x)))*count % ((int) pow(x, 2343*sin(x)));
        ftrp+=pow(sin(FUNC), 2343*sin(x*x))*(EPS);
        ftrp+=pow(sin(FUNC), 2343*sin(x*x))*(EPS);

        x+=EPS;
    }*/
    for(;;)/*count % ((int) pow(x, 2343*sin(x*x)))*count % ((int) pow(x, 2343*sin(x)))*count % ((int) pow(x, 2343*sin(x))*/;

}

/*void* burst1(){
    int x=0;
    while(1){
        FUNC;
        x++;
    }
}*/

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

    cpu_set_t mask;
    int mCpu, mCore;
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
    pthread_t thre[procNum];
    pthread_t threads[n+1];
    int processor, coreId;
    int res = -1;
    borders *bo = malloc(sizeof(borders) * n);
    borders *bb = malloc(sizeof(borders) * abs((n-procNum)));
    printf("%d", abs((n-procNum)));
    int loadCpu=  ( ((n / procNum) > (1)) ? (n / procNum) : (1) ); //per cpu
    int loadCore=2 * loadCpu;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int i=0;
    int t=0;
    int k=n%procNum*(n>procNum);
    bo[0].a = a;
    bo[0].b = a + (b - a) / n ;
    char c[33];
    mCpu=sched_getcpu();
    sprintf(c, "head -%d /proc/cpuinfo | tail -1\0", (mCpu*27+12));
    FILE* cc=popen(c, "r");
    fscanf(cc, "core id         : %d", &mCore);
    printf("bounding %d %d\n", mCpu, mCore);
    int bd=0;

    cpu_set_t masks[(int) ceil(procNum/2)];
    core* cpu=malloc(sizeof(core)*(coreIdMax+1));
    for(int y=0; y<coreIdMax+1; y++){
        cpu[y].id=-1;
        CPU_ZERO(&cpu[y].mask);
        cpu[y].load=0;
    }

    while ((res = fscanf(cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
        cpu[coreId].id=processor;
        CPU_SET(processor, &cpu[coreId].mask);
    }

    if (res != EOF) {
        perror("fscanf #1");
        return NULL;
    }
    CPU_ZERO (&mask);
    CPU_SET ((mCpu=sched_getcpu()), &mask);
    pthread_setaffinity_np (pthread_self(), sizeof (cpu_set_t), &mask);
    cpu[mCore].load=1;
    CPU_SET(mCpu, &cpu[mCore].mask);

    for(int w=0; w<=coreIdMax; w++) {
        if (cpu[w].id == -1) continue;
        while (cpu[w].load < loadCore){
            if (i < n-1) {
                bo[i].a = a + (b - a) / n * i;
                bo[i].b = a + (b - a) / n * (i + 1);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu[w].mask);
                if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread %d", errno);
                    return 0;
                }
                i++;
            } else {
                bb[t].a = bo[0].a;
                bb[t].b = bo[0].b * 100;
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu[w].mask);
                if (bd<1 || n==3) {
                    if ((pthread_create(&thre[t], &attr, burst, &bb[t])) != 0) {
                        printf("err creating thread %d", errno);
                        return 0;
                    }
                    bd += 1;
                } else {
                    if ((pthread_create(&thre[t], &attr, burst, &bb[t])) != 0) {
                        printf("err creating thread %d", errno);
                        return 0;
                    }
                }
                t++;
            }
            cpu[w].load++;
        }
        for(int u=0; u<2 && k>0; u++){ //two times
            if (i < n - 1) {
                bo[i].a = a + (b - a) / n * i;
                bo[i].b = a + (b - a) / n * (i + 1);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu[w].mask);
                if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread %d", errno);
                    return 0;
                }

                i++;
            }
            //cpu[w].load++; DO NOT UNCOMMENT
            k--;
        }
    }


    errno = 0;
    bo[i].a = a + (b - a) / n * i;
    bo[i].b = a + (b - a) / n * (i+1);
    result=result+*((double*) threadFunc(&bo[n-1]));
    double* ret[n+1];
   /* struct timespec x;
    x.tv_nsec=5;
    x.tv_sec=0;
    int d;
    for(int i=n-2; i>=0 && n>1 ; i--){
        if(!threads[i]) continue;

        if(!(d=pthread_timedjoin_np(threads[i], (void**) &ret[i], &t))){
            x.tv_nsec=5;
            x.tv_sec=0;
        }
        if(d==0){
            result+=*ret[i];
            k--;
        }
        if(i==0 && k>0) i=n-2;
    } /* Wait until thread is finished */
    for(int i=n-2; i>=0 && n>1; i--){
        if(!threads[i]) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
        free(ret[i]);
    }  /*Wait until thread is finished */
    printf("%e\n", result);
    return 0;
}