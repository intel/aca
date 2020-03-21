/****
    Copyright (C) 2019-2020 Intel Corporation.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.






****/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_NUM_THREADS 1024
#define MIN_DATA_SIZE 0x4000
#define MAX_DATA_SIZE 0x400000
#define DEF_DATA_SIZE 0x10000
#define ONE_TIME_EXECUTION 0
#define INFINITE_EXECUTION 1

static char usage[] = "USAGE: test -nt 8 [-s Byte]";
pthread_t trids[MAX_NUM_THREADS];
long dataSize = DEF_DATA_SIZE;
long primeNum[MAX_NUM_THREADS];
long *src_buffer;
unsigned int numberOfThread = 0;
unsigned int thread[MAX_NUM_THREADS];

bool isPrime(unsigned int n)
{
    if (n <= 1) return false;

    for (unsigned int i = 2; i <= n / 2; i++)
    {
        if (n % i == 0) return false;
    }

    return true;
}

void generatePrimeNum(long *primeNumArray, unsigned int primeNumArrayLength)
{
    if(!primeNumArray) return;

    unsigned int indexNaturalNum = 0;
    for(unsigned int j = 0; j < primeNumArrayLength; j++)
    {
        while(!isPrime(++indexNaturalNum))
        {
            if(indexNaturalNum == UINT32_MAX) break;
        }

        if(indexNaturalNum != UINT32_MAX)
        {
            primeNumArray[j] = indexNaturalNum;
        }
    }
}

void allocateMemory()
{
    src_buffer = (long*)malloc(sizeof(long) * dataSize);
    if (!src_buffer)
    {
        printf("Error allocating memory\n");
        printf("Try key: -s [Bytes]. DEF_DATA_SIZE = 0x%x bytes\n", DEF_DATA_SIZE);
        exit(1);
    }
}

void argParse(int argc, char** argv)
{
    if (argc >= 3 && strcmp((argv[1]),"-nt") == 0)
    {
        numberOfThread = atoi(argv[2]);
        if (argc == 5 && strcmp((argv[3]),"-s") == 0)
        {
            dataSize = atol(argv[4]);
        }
        if(numberOfThread > 0 && numberOfThread <= MAX_NUM_THREADS && dataSize >= MIN_DATA_SIZE && dataSize <= MAX_DATA_SIZE)
        {
            printf("nt = %d\n", numberOfThread);
            printf("dataSize = 0x%lx\n", dataSize);
            return;
        }
    }
    printf("%s\n",usage);
    printf("Max threads: %d\n", MAX_NUM_THREADS);
    printf("Min data size: 0x%x\n", MIN_DATA_SIZE);
    printf("Max data size: 0x%x\n", MAX_DATA_SIZE);
    exit(1);
}

size_t load(long step, long count, int flag)
{
    size_t ip;
    do
    {
        asm (
                "lea 0x3(%%rip), %%rdx;"
                "label:"
                    "movq (%[msbuf]), %%rsi;"
                    "movq %%rsi, (%[msbuf]);"
                    "leaq (%[msbuf], %[mp], 8), %[msbuf];"
                "loop label;"
            :"=d"(ip)
            :[msbuf] "a" (src_buffer), [mp] "b" (step), [mds] "c" (count)
        );
    } while(flag);
    return ip;
}

void* worker(void* threadNumArg)
{
    unsigned int threadNum = *(unsigned int*)(threadNumArg);
    long step = primeNum[threadNum];
    long count = dataSize / step;
    load(step, count, INFINITE_EXECUTION);
    return NULL;
}

void printHotspotIP()
{
    long step = 0;
    long count = 1;
    size_t ip = load(step, count, ONE_TIME_EXECUTION);
    printf("Hotspot ip: %lX : %ld\n", ip, ip);
}

void createWorkers()
{
    printf("numberOfThreads = %d\n", numberOfThread);

    for(unsigned int threadNum = 0; threadNum < numberOfThread; threadNum++)
    {
        thread[threadNum] = threadNum;
        pthread_create(&trids[threadNum], NULL, worker, &thread[threadNum]);
    }
}

void waitForWorkers()
{
    for(unsigned int threadNum = 0; threadNum < numberOfThread; threadNum++)
    {
        pthread_join(trids[threadNum], NULL);
        if (src_buffer) free(src_buffer);
    }
}

int main(int argc, char** argv)
{
    argParse(argc, argv);

    generatePrimeNum(primeNum, MAX_NUM_THREADS);

    allocateMemory();
    printHotspotIP();

    createWorkers();
    waitForWorkers();

    return 0;
}
