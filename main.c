#include "count.h"
#include <string.h>

typedef struct borders{
    double a;
    double b;
} borders;

typedef struct cpu{
    int id;
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
        printf("processor : %d\ncore id : %d\n", processor, coreId);
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
}

int getCpu(cpu* top, int ncores){
    int k=0, p=0;
    for(int i=0; i<ncores; i++){
        if(top[i].busyProces<top[k].busyProces && top[i].nproces>0) k=i;
    }
    top[k].busyProces++;
    for(int a=0; a<12; a++){
        if(top[k].proces[a]==0){
            p=k;
            break;
        }
    }
    return top[k].proces[p];
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
    //return 0;
    int ncores=0;
    cpu* topology=getCpuTopology2(&ncores);
    return 0;
    int n=input(argc, argv);
    if(!n || n<1) return -1;
    double a=1;
    double b=100;
    int k=-1;
    double result = 0;
    pthread_t threads[n];
    //int ncores=0;
    if(n>1) {
        cpu* topology=getCpuTopology2(&ncores);
        int allProcessors = sysconf(_SC_NPROCESSORS_CONF);
        if (n > allProcessors) {
            printf("Not enough proces, maximum is %d\n", allProcessors);
            return -2;
        }
        borders *bo = malloc(sizeof(borders) * allProcessors);
        pthread_attr_t attr;
        cpu_set_t mask;
        pthread_attr_init(&attr);
        int mCpu=pthread_self(), mCore=sched_getcpu();
        topology[mCore].proces[mCpu]=1;
       // int  ncores = 0;

        for (int i = 0; ncores < n - 1; i++) {
            if (i == (mCpu = sched_getcpu())) {
                k = i; //number of intertval we integrate on mother thread
                continue;
            }
            bo[i].a = a + (b - a) / n * i;
            bo[i].b = a + (b - a) / n * (i + 1);
            CPU_ZERO(&mask);
            CPU_SET(i, &mask);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
            if ((pthread_create(&threads[i], &attr, threadFunc, &bo[i])) != 0) {
                printf("err creating thread");
                return 0;
            }
            //printf("%lu created; interval [%f; %f]\n", threads[i], bo[i].a, bo[i].b);
            ncores++;
        }
    }
    if(k==-1) k=n-1;
    borders* nb=malloc(sizeof(borders));
    nb[0].a=a+(b-a)/n*(k);
    nb[0].b=a+(b-a)/n*(k+1);
    //printf("main interval [%f; %f]\n", nb[0].a, nb[0].b);
    result=result+*((double*) threadFunc(&nb[0]));
    double* ret[n];
    for(int i=n-1; i>=0 && n>1; i--){
        if(i==k) continue;
        pthread_join(threads[i], (void**) &ret[i]);
        result+=*ret[i];
    } /* Wait until thread is finished */
    printf("%e\n", result);
    return 0;
}