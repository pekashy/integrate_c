#include "count.h"
#include <string.h>

typedef struct borders{
    double a;
    double b;
} borders;

typedef struct cpu{
    int proces[12];
    int nproces;
    int busyProces;
} cpu;

double f(double x){
    return 1/(x*x*(2));
}

cpu* getCpuTopology2(int* coreNum){
    const char cheatcode[] = "fgrep -e 'processor' -e 'core id' /proc/cpuinfo";
    FILE* cpuinfo_file = popen (cheatcode, "r");
    FILE* cores=popen("grep 'cpu cores' /proc/cpuinfo", "r");
    if (!cpuinfo_file || !cores){
        perror ("error opening cpuinfo file");
        return NULL;
    }
    fscanf(cores, "cpu cores       : %d", coreNum);
    if(*coreNum<1){
        printf("core num parsing error");
        return NULL;
    }
    cpu* topology=malloc(12*sizeof(cpu));
    for(int a=0; a<12; a++){
        topology[a].nproces=0;
        topology[a].busyProces=0;
        memset(topology[a].proces, -1, 12*sizeof(int));
    }
    int processor, coreId;
    int res = -1;
    while ((res = fscanf (cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
      //  printf("processor : %d\ncore id : %d\n", processor, coreId);
        /*if(topology[coreId].nproces<0){
            topology[coreId].nproces=0;
            topology[coreId].busyProces=0;
        }*/
        topology[coreId].proces[processor]=0;
        topology[coreId].nproces++;
    }
    if (res != EOF) {
        perror ("fscanf #1");
        return NULL;
    }
    errno = 0;
    fclose (cpuinfo_file);
    fclose (cores);
    if (errno) {
        perror ("closing error");
        return NULL;
    }
    return topology;
}

int getCpu(cpu* top){
    int k=0, p=0;
    for(int i=0; i<12; i++){
        if(top[i].busyProces<top[k].busyProces && top[i].nproces>0) k=i;
    }
    top[k].busyProces++;
    for(int a=0; a<12; a++){
        if(top[k].proces[a]==0){
            p=a;
            break;
        }
    }
    top[k].proces[p]=1;
    return p;
}

int getCore(cpu* top, int ncores, int proc){
    for(int i=0; i<ncores; i++){
        if(top[i].proces[proc]>=0) return i;
    }
    return -1;
}


void* threadFunc(void* b){
    double (*fp) (double x)=f;
    borders* bord = (borders*) b;
    double FSimp=qsimp(fp, bord->a, bord->b);
    double * summ= malloc((sizeof(double)));
    *summ=FSimp;
    //printf("ID: %lu, CPU: %d\n", pthread_self(), sched_getcpu());
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
    //return 0;
    //int ncores=0;
    //cpu* topology=getCpuTopology2(&ncores);
    int n=input(argc, argv);
    if(!n || n<1) return -1;
    double a=1;
    double b=10000;
    int k=-1;
    double result = 0;
    pthread_t threads[n];
    int ncores=0;
    if(n>1) {
        cpu* topology=getCpuTopology2(&ncores);
        if(!topology){
            printf("cpu topology parsing error");
            return 0;
        }
        int allProcessors = sysconf(_SC_NPROCESSORS_CONF);
        if (n > allProcessors) {
            printf("Not enough proces, maximum is %d\n", allProcessors);
            return -2;
        }
        borders *bo = malloc(sizeof(borders) * 12);
        pthread_attr_t attr;
        cpu_set_t mask;
        pthread_attr_init(&attr);

        int mCpu=sched_getcpu(), mCore=getCore(topology, ncores, mCpu);
        topology[mCore].proces[mCpu]=1;
        topology[mCore].busyProces++;
        int proc;
        for (int i = 0; n>1 && i<allProcessors-1; i++) {
            if (mCpu != sched_getcpu()){
                topology[mCore].proces[mCpu]=0;
                topology[mCore].busyProces--;
                mCpu=sched_getcpu(), mCore=getCore(topology, ncores, mCpu);
                topology[mCore].busyProces++;
                topology[mCore].proces[mCpu]=1;
            }
            proc=getCpu(topology);
            //printf("\n%d \n", proc);
            bo[i].a = a + (b - a) / n * i;
            bo[i].b = a + (b - a) / n * (i + 1);
            CPU_ZERO(&mask);
            CPU_SET(proc, &mask);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
            if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                printf("err creating thread");
                return 0;
            }
            //printf("%lu created; interval [%f; %f]\n", threads[i], bo[i].a, bo[i].b);
            //ncores++;
        }
    }
    borders* nb=malloc(sizeof(borders));
    nb[0].a=a+(b-a)/n*(n-1);
    nb[0].b=a+(b-a)/n*(n);
    //printf("main interval [%f; %f]\n", nb[0].a, nb[0].b);
    result=result+*((double*) threadFunc(&nb[0]));
    double* ret[n];
    for(int i=n-2; i>=0 && n>1; i--){
        if(!threads[i]) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    } /* Wait until thread is finished */
    printf("%e\n", result);
    return 0;
}