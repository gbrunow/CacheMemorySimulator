//
//  main.c
//  Lab07
//
//  ECE 486/586 - Embedded Systems
//  The University of Alabama
//  Lab 07 - Spring 2015
//  Guilherme Brunow Gomes
//  11598573
//  DESCRIPTION:
//      Simulation of a cache memory with FIFO and LRU replacement policies
//
//  Created by Guilherme Brunow on 3/29/15.
//  Copyright (c) 2015 Guilherme Brunow Gomes. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define maxRefs 101
#define maxMemSize 32768
#define LRU 0
#define FIFO 1

char* intToBin(int, int);
double bestRate(int*, int, int);
int has(int*, int, int);
char* getTag(char*, int, int);
int isPowerOfTwo(int);
int readInteger(int*);

void printCache(int, int, int);

struct location{
    int cacheSet;
    int dirty;
    int valid;
    char* tag;
    int mmBlk;
    int addr;
    int queuePosition;
};

int cacheHandler(struct location, int, int, int, int, int);

struct location cache[maxMemSize];

int main(int argc, const char * argv[])
{
    FILE *file;
    char fileName[100];
    char mode = '0';
    int memLocation = -1;
    int memRefs = -1;
    int locationsDec[maxRefs];
    char* locationBin;
    int memSize;
    int cacheSize;
    int blockSize;
    int associativeDegree;
    int replacementPolicy;
    
//    int step_by_step = 1;
    int isInteger;
    char command;
    
    do
    {
        
        /*--------- USER INPUT SECTION ---------*/
        
        printf("Memory size: ");
        isInteger = readInteger(&memSize);
        while(isInteger == 0|| !isPowerOfTwo(memSize) || memSize > maxMemSize){
            printf("Invalid input.\n");
            printf("Memory size: ");
            isInteger = readInteger(&memSize);
        }
        
        printf("Cache size: ");
        isInteger = readInteger(&cacheSize);
        while(isInteger == 0 || !isPowerOfTwo(cacheSize) || cacheSize>memSize){
            printf("Invalid input.\n");
            printf("Cache size: ");
            isInteger = readInteger(&cacheSize);
        }
        
        printf("Block size: ");
        isInteger = readInteger(&blockSize);
        while(isInteger == 0|| !isPowerOfTwo(blockSize) || blockSize>cacheSize){
            printf("Invalid input.\n");
            printf("Block size: ");
            isInteger = readInteger(&cacheSize);
        }
        
        printf("Associative degree: ");
        isInteger = readInteger(&associativeDegree);
        while(isInteger == 0|| !isPowerOfTwo(associativeDegree) || associativeDegree<1 || associativeDegree>(cacheSize/blockSize)){
            printf("Invalid input.\n");
            printf("Associative size: ");
            isInteger = readInteger(&associativeDegree);
        }
        
        printf("Replacement policy \n");
        printf("\t[0] - LRU\n\t[1] - FIFO\n");
        printf("Option: ");
        isInteger = readInteger(&replacementPolicy);
        while(isInteger == 0 || (replacementPolicy != 0 && replacementPolicy != 1)){
            printf("Invalid input.\n");
            printf("Replacement policy \n");
            printf("\t[0] - LRU\n\t[1] - FIFO\n");
            printf("Option: ");
            isInteger = readInteger(&replacementPolicy);
        }
        
        /*--------- CALCULATION OF MEMORY ASPECTS SECTION ---------*/
        
        int addressLines = log2(memSize);
        int offsetSize = log2(blockSize);
        int indexSize = log2(cacheSize/(associativeDegree*blockSize));
        int tagSize = addressLines - indexSize - offsetSize;
        int totalCache = cacheSize + (tagSize + 2)*cacheSize/(blockSize*8);
        
        printf("\nAddress lines:\t\t\t%6d\n", addressLines);
        printf("Number of offset bits:\t%6d\n",offsetSize);
        printf("Number of index bits:\t%6d\n",indexSize);
        printf("Number of tag bits:\t\t%6d\n",tagSize);
        printf("Total cache size:\t\t%6d bytes\n\n", totalCache);
        
        /*--------- FILE VALIDATION AND "EXECUTION" SECTION ---------*/
        
        int hits = 0;
        char* hit_vec[maxRefs];
        int fileValid;
        do
        {
            fileValid = 0;
            printf("File name: ");
            scanf("%s", fileName);
            
            file = fopen(fileName, "r");
            
            if (file == NULL)
            {
                printf("Can't open input file,\n");
                fclose(file);
                continue;
            }
            
            //read the first line of the file
            if(fscanf(file, "%d\r", &memRefs) == EOF)
            {
                printf("This file is not valid.\n");
                fclose(file);
                continue;
            }
            
            int i = 0;
            
            //read the rest of the file line by line
            while (fscanf(file, "%c %d\r", &mode, &memLocation) != EOF)
            {
                if((mode != 'R' && mode != 'r' && mode != 'W' && mode != 'w') || memLocation < 0 || memLocation > memSize-1)
                {
                    i = -1;
                    break;
                }
                locationsDec[i] = memLocation;
                locationBin = intToBin(locationsDec[i], addressLines);
                
                struct location cacheLocation;
                cacheLocation.addr = memLocation;
                cacheLocation.valid = 1;
                if(mode == 'w'|| mode == 'W')
                    cacheLocation.dirty = 1;
                else
                    cacheLocation.dirty = 0;
                cacheLocation.tag = getTag(locationBin, addressLines, tagSize);
                cacheLocation.mmBlk = memLocation/blockSize;
                cacheLocation.queuePosition = 1;
                cacheLocation.cacheSet = cacheLocation.mmBlk%(cacheSize/blockSize/associativeDegree);
                
                int hit = cacheHandler(cacheLocation, memSize, cacheSize, associativeDegree, blockSize, replacementPolicy);
                hits += hit;
                
//                if(step_by_step == 1)
//                    printCache(tagSize, cacheSize, blockSize);
                
                if(hit == 1){
                    hit_vec[i] = "hit";
                }
                else
                {
                    hit_vec[i] = "miss";
                }
                
                mode = '0';
                memLocation = -1;
                i++;
            }
            
            if(i != memRefs)
            {
                printf("This file is not valid.\n");
                fclose(file);
                continue;
            }
            fileValid = 1;
        }while(!fileValid);
        
        fclose(file);
        
        /*--------- RESULTS OUTPUT SECTION ---------*/
        
        printf("\n----------------------------------------------------------------------------\n");
        printf("\tMM addr\t|\t MM blk #\t|\tCM set #\t|\t CM blk#\t|\tHit/Miss\n");
        printf("----------------------------------------------------------------------------\n");
        
        for(int i = 0; i<memRefs; i++)
        {
            int mmBlk = locationsDec[i]/blockSize;
            int blksNumber = cacheSize/blockSize;
            int cacheSet = mmBlk%(blksNumber/associativeDegree);
            printf("%10d\t|\t", locationsDec[i]);
            printf("% 9d\t|\t", mmBlk);
            printf("% 9d\t|\t", cacheSet);
            if(associativeDegree > 1)
                printf("%3d - %d%3\t|\t", cacheSet*associativeDegree, (cacheSet+1)*associativeDegree-1);
            else
                printf("%5d\t\t|\t", cacheSet*associativeDegree);
            printf("%6s\n", hit_vec[i]);
        }
        
        printf("----------------------------------------------------------------------------\n");
        
        double bestPossibleRate = bestRate(locationsDec, memRefs, blockSize);
        
        printf("\nHighest possible hit rate:\t%d/%d = %.2f%%\n", (int)(memRefs*bestPossibleRate), memRefs, bestPossibleRate*100);
        printf("Actual hit rate:\t\t\t%d/%d = %.2f%%\n\n", hits, memRefs, (double)((double)hits/(double)memRefs)*100);
        
        printCache(tagSize, cacheSize, blockSize);
        
        fseek(stdin,0,SEEK_END); //clean buffers
        printf("Continue?\n\t[Y] - Yes\n\t[N] - No\n");
        char end;
        scanf("%c%c", &command, &end);
        while((command != 'y' && command != 'Y' && command != 'n'&& command != 'N') || end != '\n'){
            fseek(stdin,0,SEEK_END); //clean buffers
            printf("Invalid input.\n");
            printf("Continue?\n\t[Y] - Yes\n\t[N] - No\n");
            scanf("%c%c", &command, &end);
        }
    }while(command == 'y'|| command == 'Y');
    return 0;
}

double bestRate(int *locationsDec, int memRefs, int blockSize)
{
    int uniques[maxRefs];
    int mmBlks[maxRefs];
    int k = 0;
    int possibleHits = 0;
    for(int i = 0; i<memRefs; i++)
    {
        int addr = *(locationsDec+i);
        int mmBlk = (int)addr/blockSize;
        if(has(uniques, mmBlk, k)==0)
        {
            uniques[k] = mmBlk;
            k++;
        }
        mmBlks[i]=mmBlk;
    }
    
    for(int i=0; i<k; i++)
    {
        possibleHits += has(mmBlks, uniques[i], memRefs) -1;
    }
    
    return (double)((double)possibleHits/(double)memRefs);
}

char* intToBin(int number, int bits)
{
    char *binary = (char*)malloc(bits+1);
    if (binary == NULL)
        return -1;
    
    for (int i = 0; i < bits; i++)
    {
        int bit = number >> i;
        if (bit & 1)
            *(binary+i) = '1';
        else
            *(binary+i) = '0';
    }
    for(int i=0; i<bits/2; i++)
    {
        char temp = *(binary+i);
        *(binary+i) = *(binary+bits-i-1);
        *(binary+bits-i-1) = temp;
    }
    *(binary+bits) = '\0';
    return binary;
}

int has(int *vector, int mmBlk, int size)
{
    int count = 0;
    for(int i=0; i<size; i++)
    {
        if(vector[i] == mmBlk)
        {
            count++;
        }
    }
    return count;
}

char* getTag(char* location, int addressLines, int tagSize)
{
    char *tag = (char*)malloc(tagSize+1);
    for(int i = 0; i<tagSize; i++)
    {
        *(tag+i) = *(location+i);
        
    }
    *(tag+tagSize)='\0';
    return tag;
}

int cacheHandler(struct location memLocation, int memSize, int cacheSize, int associativeDegree, int blockSize, int replacementPolicy)
{
    
    int hit = 0;
    //navigates the whole cache set
    for(int i = memLocation.cacheSet*associativeDegree; i<(memLocation.cacheSet+1)*associativeDegree; i++)
    {
        //if hit or there is space
        if(cache[i].valid == 0 || cache[i].mmBlk == memLocation.mmBlk )
        {
            if(cache[i].valid == 1 && cache[i].mmBlk == memLocation.mmBlk )
            {
                hit = 1;
            }
            
            //puts the hit/new location at the end of the queue
            for(int j = memLocation.cacheSet; j<memLocation.cacheSet+associativeDegree; j++)
            {
                //puts the hit location at the en of the queue - only for LRU method
                if(cache[j].queuePosition-1 > memLocation.queuePosition && replacementPolicy == LRU)
                {
                    int tempQueuePosition = cache[j].queuePosition-1;
                    cache[j].queuePosition = memLocation.queuePosition;
                    memLocation.queuePosition = tempQueuePosition;
                }
                else{
                    //finds the queue end and places the new location on it
                    if(cache[j].queuePosition >= memLocation.queuePosition && cache[j].queuePosition < associativeDegree)
                    {
                        memLocation.queuePosition = cache[j].queuePosition + 1;
                    }
                }
            }
            
            //update cache if there was no hit
            if(hit != 1 || replacementPolicy == LRU)
                cache[i] = memLocation;
            
            return hit;
        }
    }
    
    //replacement routine
    int firstInQueue = -1;
    while(firstInQueue == -1)
    {
        for(int i = memLocation.cacheSet*associativeDegree; i<(memLocation.cacheSet+1)*associativeDegree; i++)
        {
            if(cache[i].queuePosition == 0)
            {
                firstInQueue = i;
                continue;
            }
            cache[i].queuePosition--;
        }
    }
    
    //puts the location at the end of the queue
    for(int i = memLocation.cacheSet*associativeDegree; i<(memLocation.cacheSet+1)*associativeDegree; i++)
    {
        if(cache[i].queuePosition > memLocation.queuePosition)
        {
            int tempQueuePosition = cache[i].queuePosition;
            cache[i].queuePosition = memLocation.queuePosition;
            memLocation.queuePosition = tempQueuePosition;
        }
    }
    cache[firstInQueue] = memLocation;
    
    return hit;
}

void printCache(int tagSize, int cacheSize, int blockSize)
{
    char dontcare[50];
    
    for(int i=0; i<tagSize; i++)
    {
        dontcare[i] = 'x';
        dontcare[i+1] = '\0';
    }
    
    printf("\n-----------------------------------------------------------------------\n");
    printf("\tD\t|\tV\t|%7Tag%7|\t\tData\t\t|\tCache blk #\n");
    printf("-----------------------------------------------------------------------\n");
    
    for(int j=0; j<cacheSize/blockSize; j++)
    {
        printf("\t%d\t|\t", cache[j].dirty);
        printf("%d\t|\t", cache[j].valid);
        if(cache[j].valid == 1)
        {
            printf("%7s%3\t|\t", cache[j].tag);
            printf("MM blk # %4d\t|\t",cache[j].mmBlk);
        }
        else
        {
            printf("%7s%3\t|\t\t  X %9|\t", dontcare);
        }
        printf("%6d\n",j);
    }
    
    printf("-----------------------------------------------------------------------\n");
    
}

//determines if a given number is power of two
int isPowerOfTwo(int number){
    double base = log2(number);                 //opositive operation of power of 2
    return ceilf(base) == base || number == 2;  //if the log2 result is an integer return true (1)
}

//reads standard input and stores reading return whether the data is valid (1) or not (0)
int readInteger(int *result){
    int num;
    char term;
    if(scanf("%d%c", &num, &term) != 2 || term != '\n')
    {
        fseek(stdin,0,SEEK_END); //clean buffers
        //printf("Error, not an integer!\n");
        return 0;
    }
    *result = num;
    return 1;
}
