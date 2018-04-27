#include "count.h"
#include <string.h>
#include <search.h>
#include <sys/sysinfo.h>

typedef struct borders{
    double a;
    double b;
} borders;

typedef struct cpu{
    int proces[12];
   // int* links[2];
    int nproces;
    int busyProces;
} cpu;

typedef struct processor{
    int id;
    struct core* aff;
    int loaded;
} proc;

typedef struct core{
    proc* procs[2];
    int load;
} core;

double f(double x){
    return 1/(x*x*(2));
}

cpu* getCpuTopology2(int* coreIdMax, int procsNum, int n, int a, int b){
    const char cheatcode[] = "fgrep -e 'processor' -e 'core id' /proc/cpuinfo";
    FILE* cpuinfo_file = popen (cheatcode, "r");
    FILE* crs=popen("grep 'core id' /proc/cpuinfo | grep -Eo '[0-9]{1,4}' | sort -rn | head -n 1", "r"); //getting maximum cpuId
    if (!cpuinfo_file || !crs){
        perror ("error opening cpuinfo file");
        return NULL;
    }
    fscanf(crs, "%d", coreIdMax);
    if(*coreIdMax<1){
        printf("core num parsing error");
        return NULL;
    }
    core* cores=malloc(sizeof(core)*12);
    proc* procs=malloc(sizeof(proc)*procsNum);

    for(int g=0; g<*coreIdMax; g++){
        cores[g].procs[0]=NULL;
        cores[g].procs[1]=NULL;
        cores[g].load=0;
    }
    for(int g=0; g<procsNum; g++){//can be removed assuming we have hyperthr
        procs[g].id=b;
        procs[g].loaded=0;
        procs[g].aff=NULL;
    }

    pthread_attr_t attr;
    cpu_set_t mask;
    pthread_attr_init(&attr);

    int processor, coreId;
    int res = -1;
    int mainP=sched_getcpu();

    borders *bo = malloc(sizeof(borders) * n);
    pthread_t threads[n];
    int i=0, s=0;
    int queue[n];
    memset(n, -1, n*sizeof(int));

    while ((res = fscanf (cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
      //  printf("processor : %d\ncore id : %d\n", processor, coreId);
        procs[processor].aff=&cores[coreId];

        if(!cores[coreId].procs[0]) //bounding
            cores[coreId].procs[0]=&procs[processor];
        else
            cores[coreId].procs[1]=&procs[processor];
        if(i<n) {
            bo[i].a = a + (b - a) / n * i;
            bo[i].b = a + (b - a) / n * (i + 1);

            if (processor == sched_getcpu()) {//works with first
                CPU_ZERO(&mask);
                CPU_SET(mainP, &mask); //sending thread to the last main location
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                procs[processor].loaded = 1;//exchange current for main, load is the same for both
                cores[coreId].load++;
                mainP = sched_getcpu();
                i++;
                continue;
            }

            if (cores[coreId].load < procsNum / 2) {

                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                procs[processor].loaded = 1;
                cores[coreId].load++;
            } else { //storing in queue
                queue[s] = i;
                s++;
            }
            i++;
        }
        else{ //running stored in queue
            if (processor == sched_getcpu()) {//main thread changed its location
                CPU_ZERO(&mask);
                CPU_SET(mainP, &mask); //sending thread to the last main location
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&threads[s], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                procs[processor].loaded = 1;//exchange current for main, load is the same for both
                cores[coreId].load++;
                mainP = sched_getcpu();
                s--;
                continue;
            }
            if (cores[coreId].load < procsNum / 2) {
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if ((pthread_create(&threads[s], &attr, threadFunc, &bo[i])) != 0) {
                    printf("err creating thread");
                    return 0;
                }
                procs[processor].loaded = 1;
                cores[coreId].load++;
                s--;
            }
         }
    //run left in queue
    for(int b=0; b<*coreIdMax && s>0; b++) { //all should have load at least 1 now
        if(!cores[b].procs[0]) continue;
        coreId=b;
        processor=cores[b].procs[cores[b].load][s].id;

        if (processor == sched_getcpu()) {//main thread changed its location
            CPU_ZERO(&mask);
            CPU_SET(mainP, &mask); //sending thread to the last main location
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
            if ((pthread_create(&threads[s], &attr, threadFunc, &bo[i])) != 0) {
                printf("err creating thread");
                return 0;
            }
            procs[processor].loaded = 1;//exchange current for main, load is the same for both
            cores[coreId].load++;
            mainP = processor;
            s--;
            continue;
        }
        if(cores[b].load<2){
            CPU_ZERO(&mask);
            CPU_SET(processor, &mask);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
            if ((pthread_create(&threads[s], &attr, threadFunc, &bo[i])) != 0) {
                printf("err creating thread");
                return 0;
            }
            procs[processor].loaded = 1;
            cores[coreId].load++;
            s--;
        }
    }
        if (res != EOF) {
        perror ("fscanf #1");
        return NULL;
    }
    errno = 0;
    fclose (cpuinfo_file);
    fclose (crs);
    if (errno) {
        perror ("closing error");
        return NULL;
    }
    //return topology;
}

void* threadFunc(void* b){
    double (*fp) (double x)=f;
    borders* bord = (borders*) b;
    double FSimp=qsimp(fp, bord->a, bord->b);
    double * summ= malloc((sizeof(double)));
    *summ=FSimp;
    printf("ID: %lu, CPU: %d\n", pthread_self(), sched_getcpu());
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
    double a=25000;
    double b=100000;
    int k=-1;
    double result = 0;
    pthread_t threads[n];
    int ncores=0;
    borders *bo = malloc(sizeof(borders) * n);
    if(n>1) {
        int nprocs = get_nprocs();
        cpu* topology=getCpuTopology2(&ncores, nprocs);
        if(!topology){
            printf("cpu topology parsing error");
            return 0;
        }
        if (n > nprocs) {
            printf("Not enough proces, maximum is %d\n", nprocs);
            return -2;
        }

        pthread_attr_t attr;
        cpu_set_t mask;
        pthread_attr_init(&attr);

        int mCpu=sched_getcpu(), mCore=getCore(topology, ncores, mCpu);
        topology[mCore].proces[mCpu]=1;
        topology[mCore].busyProces++;
        int proc;
        for (int i = 0; n>1 && i<n-1; i++) {
            if (mCpu != sched_getcpu()){
                printf("WUMP WUMP MAIN ESCAPED\n");
                topology[mCore].proces[mCpu]=0;
                topology[mCore].busyProces--;
                mCpu=sched_getcpu(), mCore=getCore(topology, ncores, mCpu);
                topology[mCore].busyProces++;
                topology[mCore].proces[mCpu]=1;
            }
            proc=getCpu(topology, nprocs);
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
    bo[n-1].a=a+(b-a)/n*(n-1);
    bo[n-1].b=a+(b-a)/n*(n);
    result=result+*((double*) threadFunc(&bo[n-1]));
    double* ret[n];
    for(int i=n-2; i>=0 && n>1; i--){
        if(!threads[i]) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    } /* Wait until thread is finished */
    printf("%e\n", result);
    return 0;
}