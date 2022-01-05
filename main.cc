/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <cstring>
#include <fstream>
using namespace std;

#include "cache.h"

int global_protocol;

int main(int argc, char *argv[])
{

    ifstream fin;
    FILE * pFile;
    unsigned long address;
    unsigned long processor_id;
    char str[2];
    unsigned char rw;
    int i;

    if(argv[1] == NULL){
        printf("input format: ");
        printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
        exit(0);
    }

    int cache_size = atoi(argv[1]);
    int cache_assoc= atoi(argv[2]);
    int blk_size   = atoi(argv[3]);
    int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
    int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
    global_protocol = protocol;
    char *fname =  (char *)malloc(20);
    fname = argv[6];
    char protocol_name[10];
    switch(protocol) {
        case 0:
            std::strcpy(protocol_name, "MSI");
            break;
        case 1:
            std::strcpy(protocol_name, "MESI");
            break;
        case 2:
            std::strcpy(protocol_name, "Dragon");
            break;
        default:
            cout << "Incorrect protocol given!" << endl;
            break;
    }

    cout    << "===== 506 Personal information =====" << endl
            << "Ian (Robert) Hellmer" << endl
            << "crhellme" << endl
            << "undergrad section? NO" << endl;

    cout    << "===== 506 SMP Simulator configuration =====" << endl
            << "L1_SIZE: " << cache_size << endl
            << "L1_ASSOC: " << cache_assoc << endl
            << "L1_BLOCKSIZE: " << blk_size << endl
            << "NUMBER OF PROCESSORS: " << num_processors << endl
            << "COHERENCE PROTOCOL: " << protocol_name << endl
            << "TRACE FILE: " << fname << endl;

    //*********************************************//
    //*****create an array of caches here**********//
    //*********************************************//

    pFile = fopen (fname,"r");
    if(pFile == 0)
    {
        printf("Trace file problem\n");
        exit(0);
    }

    // Initialize array of caches to be used for simulation
    Cache **cacheArray;
    cacheArray = new Cache*[num_processors];
    for(i = 0; i < num_processors; i++) {
        cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
    }

    //**read trace file,line by line,each(processor#,operation,address)**//
    //*****propagate each request down through memory hierarchy**********//
    //*****by calling cachesArray[processor#]->Access(...)***************//
    while(fscanf(pFile, "%lu %s %lx", &processor_id, str, &address) != EOF) {
        rw = str[0];
        cacheArray[processor_id]->Access(address, rw, cacheArray, num_processors, processor_id);
    }
    fclose(pFile);

    //********************************//
    //print out all caches' statistics//
    //********************************//
    for(i = 0; i < num_processors; i++) {
        cacheArray[i]->printStats(i);
    }

}
