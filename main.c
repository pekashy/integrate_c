#include "count.h"
#include <string.h>
#include <search.h>
#include <sys/sysinfo.h>

typedef struct borders {
    double a;
    double b;
} borders;

typedef struct cpu {
    int proces[12];
    // int* links[2];
    int nproces;
    int busyProces;
} cpu;

typedef struct processor {
    int id;
    int aff;
    int loaded;
} proc;

typedef struct core {
    proc *procs[2];
    int load;
} core;

double f(double x) {
    return 1 / (x);
}

void *threadFunc(void *b) {
    double (*fp)(double x)=f;
    borders *bord = (borders *) b;
    //printf(" %f %f ID: %lu, CPU: %d\n", bord->a, bord->b, pthread_self(), sched_getcpu());

    double FSimp = qsimp(fp, bord->a, bord->b);
    double *summ = malloc((sizeof(double)));
    *summ = FSimp;
    printf("ID: %lu, CPU: %d\n", pthread_self(), sched_getcpu());
    return summ;
}

int input(int argc, char **argv) {
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

int main(int argc, char *argv[]) {
    int n = input(argc, argv);
    if (!n || n < 1) return -1;
    double a = 25000;
    double b = 100000000;
    borders *bo = malloc(sizeof(borders) * n);
    pthread_t threads[n];
    memset(threads, 0, n*sizeof(pthread_t));
    double result = 0;

    //int k=-1;
    int coreIdMax = -1;
    int procsNum = get_nprocs();
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
    core *cores = malloc(sizeof(core) * (coreIdMax+1));
    proc *procs = malloc(sizeof(proc) * procsNum);

    for (int g = 0; g <= coreIdMax; g++) {
        cores[g].procs[0] = NULL;
        cores[g].procs[1] = NULL;
        cores[g].load = 0;
    }
    for (int g = 0; g < procsNum; g++) {//can be removed assuming we have hyperthr
        procs[g].id = g;
        procs[g].loaded = 0;
        procs[g].aff = NULL;
    }

    pthread_attr_t attr;
    cpu_set_t mask;
    pthread_attr_init(&attr);

    int processor, coreId;
    int res = -1;
    int mainP = sched_getcpu();
    procs[mainP].loaded = 1; //main taken a thread

    int i = 0, s = 0;
    int queue[n];
    memset(queue, -1, n * sizeof(int));

    while ((res = fscanf(cpuinfo_file, "processor : %d\ncore id : %d\n", &processor, &coreId)) == 2) {
        printf("processor : %d\ncore id : %d\n", processor, coreId);
        procs[processor].aff = coreId;
        if(procs[processor].loaded) cores[coreId].load++; //we have main thread on cpu

        if (!cores[coreId].procs[0]) //bounding
            cores[coreId].procs[0] = &procs[processor];
        else
            cores[coreId].procs[1] = &procs[processor];
        if (i < n - 1) {
            bo[i].a = a + (b - a) / n * i;
            bo[i].b = a + (b - a) / n * (i + 1);

            if (processor == sched_getcpu()) {//works with first
                if (processor == mainP || procs[mainP].loaded) { //the process was taken by main from the begining
                    if(procs[mainP].loaded==0) cores[coreId].load++;
                    procs[mainP].loaded = 1; //main taken a thread

                    printf("THREAD STORED IN QUEUE BY MAIN\n");
                    queue[s] = i;
                    s++;
                }
                else {
                    CPU_ZERO(&mask);
                    CPU_SET(mainP, &mask); //sending thread to the last main location
                    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                    if (pthread_create(&threads[i], &attr, threadFunc, &bo[i])) {
                        printf("err creating thread");
                        return 0;
                    }
                    if(procs[mainP].loaded==0) cores[procs[mainP].aff].load++;
                    procs[mainP].loaded = 1;//setting thread to the old spot of the main
                    //procs[mainP].loaded = 0; //main freed a thread
                    //cores[procs[mainP].aff].load--;
                    //if(!threads[i]) threads[i]=1; //some kind of bug: returns 0 thread id
                    printf("%lu THREAD STARTED IN CYCLE MAIN mainP: %d processor: %d\n", threads[i], mainP, processor);
                    //mainP = sched_getcpu(); //getting new location for main
                    if(procs[processor].loaded==0) cores[procs[processor].aff].load++;
                    procs[processor].loaded = 1; //main taken new thread

                }
                i++;
                continue;
            }
            else if (cores[coreId].load < procsNum / 2) {
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if (pthread_create(&threads[i], &attr, threadFunc, &bo[i])) {
                    printf("err creating thread");
                    return 0;
                }
                procs[processor].loaded = 1;
                cores[coreId].load++;
                //if(!threads[i]) threads[i]=1; //some kind of bug: returns 0 thread id
                printf("%lu load: %d THREAD STARTED IN CYCLE %d\n",threads[i], cores[coreId].load, processor);

            } else { //storing in queue
                printf("THREAD STORED IN QUEUE\n");

                queue[s] = i;
                s++;
            }
            i++;
        } else if (s > 0) { //running stored in queue
            printf("%d\n", s);
            /*if (processor == sched_getcpu()) {//main thread changed its location
                CPU_ZERO(&mask);
                CPU_SET(mainP, &mask); //sending thread to the last main location
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if (pthread_create(&threads[s], &attr, threadFunc, &bo[s])) {//sending sth from the queue
                    printf("err creating thread");
                    return 0;
                }
                procs[processor].loaded = 1;//exchange current for main, load is the same for both
                cores[coreId].load++;
                mainP = sched_getcpu();
                queue[s] = -1;
                s--;
                if(!threads[i]) threads[i]=1; //some kind of bug: returns 0 thread id
                printf("%lu THREAD STARTED IN QUEUE CYCLE MAIN %d\n",threads[s], processor);
                continue;

            }*/
            if (cores[coreId].load < procsNum / 2 && processor != sched_getcpu()) {
                CPU_ZERO(&mask);
                CPU_SET(processor, &mask);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
                if (pthread_create(&threads[queue[s-1]], &attr, threadFunc, &bo[queue[s-1]])) {//sending sth from the queue
                    printf("err creating thread");
                    return 0;
                }
                procs[processor].loaded = 1;
                //if(!threads[i]) threads[i]=1; //some kind of bug: returns 0 thread id
                cores[coreId].load++;
                queue[s-1] = -1;
                printf("%lu THREAD STARTED IN QUEUE CYCLE %d\n",threads[queue[s-1]], processor);
                s--;
            }
        }
    }
    if (res != EOF && n > 1) {
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

    //run left in queue
    for (int q = 0; q <= coreIdMax && s > 0; q++) { //all should have load at least 1 now
        if (!cores[q].procs[0]) continue; //no core with this id
        if (cores[q].load == 2) continue; //all core is taken
        coreId = q;
        //processor = cores[q].procs[cores[q].load][s].id;
        if (!cores[q].procs[0]->loaded) processor = cores[q].procs[0]->id;
        else processor = cores[q].procs[1]->id;
        printf("%d processor: %d cpu1loaded?: %d cpu2loaded?: %d\n", cores[q].load, processor,
               procs[cores[q].procs[0]->id].loaded, procs[cores[q].procs[1]->id].loaded);
        if (processor == sched_getcpu()) {//main thread changed its location
            if(procs[mainP].loaded)cores[procs[mainP].aff].load++;
            procs[mainP].loaded=0;
            if(!procs[processor].loaded) cores[procs[processor].aff].load++;
            procs[processor].loaded=1;
            mainP=sched_getcpu();
        } else {
            /*CPU_ZERO(&mask);
            CPU_SET(mainP, &mask); //sending thread to the last main location
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
            if ((pthread_create(&threads[s], &attr, threadFunc, &bo[s])) != 0) {
                printf("err creating thread");
                return 0;
            }
            printf("THREAD STARTED IN QUEUE MAIN %d\n", mainP);
            procs[processor].loaded = 1;//exchange current for main, load is the same for both
            cores[coreId].load++;
            mainP = processor;
            s--;
            continue;*/

            CPU_ZERO(&mask);
            CPU_SET(processor, &mask);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
            if ((pthread_create(&threads[queue[s - 1]], &attr, threadFunc, &bo[queue[s - 1]])) != 0) {
                printf("err creating thread");
                return 0;
            }

            procs[processor].loaded = 1;
            cores[coreId].load++;
            s--;
            printf("THREAD STARTED IN QUEUE %d\n", processor);
        }
        if (q == coreIdMax && s > 0) q = -1;
    }


    bo[n - 1].a = a + (b - a) / n * (n - 1);
    bo[n - 1].b = a + (b - a) / n * (n);
    struct timespec t;
    t.tv_nsec = 100;
    t.tv_sec = 0;
    result = result + *((double *) threadFunc(&bo[n - 1]));
    double *ret[n];
    int k = n-1;
    int d;
    /*for (int r = 0; r <= n && k>0; r++) {
        //if(!threads[i]) continue;
        if (threads[r]) {
            if (!(d = pthread_timedjoin_np(threads[r], (void **) &ret[r], &t))) {
                t.tv_nsec = 10000;
                t.tv_sec = 0;
            }
            if (d == 0) {
                result += *ret[r];
                threads[r] = NULL;
                k--;
            }
        }
        if (r == n && k > 0)
            r = -1;
            */
    for(int r=0; r<n && n>1; r++){//nonblocking join makes no sense
        //if(!threads[r]) continue;
        pthread_join(threads[r], (void**) &ret[r]);
        result+=*ret[r];
    } /* Wait until thread is finished */
    printf("%e\n", result);
    return 0;
}