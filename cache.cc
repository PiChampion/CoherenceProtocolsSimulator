/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <iomanip>      // std::setprecision
#include "cache.h"
using namespace std;

extern int global_protocol;

Cache::Cache(int s,int a,int b )
{
    ulong i, j;
    reads = readMisses = writes = 0;
    writeMisses = writeBacks = currentCycle = 0;
    transfers = memoryTransactions = interventions = 0;
    invalidations = flushes = busrdx = 0;

    size       = (ulong)(s);
    lineSize   = (ulong)(b);
    assoc      = (ulong)(a);
    sets       = (ulong)((s/b)/a);
    numLines   = (ulong)(s/b);
    log2Sets   = (ulong)(log2(sets));
    log2Blk    = (ulong)(log2(b));

    tagMask =0;
    for(i=0;i<log2Sets;i++)
    {
        tagMask <<= 1;
        tagMask |= 1;
    }

    /**create a two dimensional cache, sized as cache[sets][assoc]**/
    cache = new cacheLine*[sets];
    for(i=0; i<sets; i++)
    {
        cache[i] = new cacheLine[assoc];
        for(j=0; j<assoc; j++)
        {
            cache[i][j].invalidate();
        }
    }

}

/**you might add other parameters to Access()
since this function is an entry point
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr, uchar op, Cache ** CacheArray, int processors, unsigned long processor_id)
{
    int i;

    currentCycle++;/*per cache global counter to maintain LRU order
			among cache ways, updated on every cache access*/

    if(op == 'w') writes++;
    else          reads++;

    cacheLine *line = findLine(addr);
    cacheLine *newline;
    bool C, flush_opt_received;

    switch(global_protocol) {
        case 0:     // MSI protocol
            if (line == NULL) /*miss*/
            {
                if (op == 'w') {
                    writeMisses++;
                    busrdx++;
                } else readMisses++;

                for (i = 0; i < processors; i++) {
                    if ((unsigned)i != processor_id) {
                        CacheArray[i]->Snoop(addr, op);
                    }
                }

                newline = fillLine(addr);
                if (op == 'w') newline->setFlags(MODIFIED);
                memoryTransactions++;
            }
            else { /*hit*/
                /**since it's a hit, update LRU and update dirty flag**/
                updateLRU(line);
                if (op == 'w') {
                    if (line->getFlags() == SHARED) {
                        for (i = 0; i < processors; i++) {
                            if ((unsigned)i != processor_id) {
                                CacheArray[i]->Snoop(addr, op);
                            }
                        }
                        busrdx++;
                        memoryTransactions++;
                    }
                    line->setFlags(MODIFIED);
                }
            }
            break;
        case 1:     // MESI protocol
            flush_opt_received = false;
            C = false;

            if (line == NULL) /*miss*/
            {
                if (op == 'w') {
                    writeMisses++;
                    busrdx++;
                } else readMisses++;

                for (i = 0; i < processors; i++) {
                    if ((unsigned)i != processor_id) {
                        if(CacheArray[i]->Snoop(addr, op)) flush_opt_received = true;
                        if(CacheArray[i]->findLine(addr)) C = true;
                    }
                }

                newline = fillLine(addr);
                if (!C && op == 'r') newline->setFlags(EXCLUSIVE);
                else if (op == 'w') newline->setFlags(MODIFIED);
                else if (C && op == 'r') newline->setFlags(SHARED);

                if(flush_opt_received) transfers++;
                else memoryTransactions++;
            }
            else { /*hit*/
                /**since it's a hit, update LRU and update dirty flag**/
                updateLRU(line);
                if (op == 'w') {
                    if (line->getFlags() == SHARED) {
                        for (i = 0; i < processors; i++) {
                            if ((unsigned)i != processor_id) {
                                CacheArray[i]->Snoop(addr, 'u');
                            }
                        }
                    }
                    line->setFlags(MODIFIED);
                }
            }
            break;
        case 2:     // Dragon protocol
            C = false;

            if (line == NULL) /*miss*/
            {
                if (op == 'w') {
                    writeMisses++;
                } else readMisses++;

                for (i = 0; i < processors; i++) {
                    if ((unsigned)i != processor_id) {
                        CacheArray[i]->Snoop(addr, 'r');
                        if(CacheArray[i]->findLine(addr)) C = true;
                    }
                }

                newline = fillLine(addr);
                if(!C && op == 'r') newline->setFlags(EXCLUSIVE);
                else if (!C && op == 'w') newline->setFlags(MODIFIED);
                else if (C && op == 'r') newline->setFlags(SHAREDCLEAN);
                else if (C && op == 'w') {
                    newline->setFlags(SHAREDMODIFIED);
                    for (i = 0; i < processors; i++) {
                        if ((unsigned)i != processor_id) {
                            CacheArray[i]->Snoop(addr, 'u');
                        }
                    }
                }
                memoryTransactions++;
            }
            else { /*hit*/
                /**since it's a hit, update LRU and update dirty flag**/
                updateLRU(line);
                if (op == 'w') {
                    if (line->getFlags() == SHAREDMODIFIED || line->getFlags() == SHAREDCLEAN) {
                        for (i = 0; i < processors; i++) {
                            if ((unsigned)i != processor_id) {
                                CacheArray[i]->Snoop(addr, 'u');
                                if(CacheArray[i]->findLine(addr)) C = true;
                            }
                        }
                        if(!C) line->setFlags(MODIFIED);
                        else line->setFlags(SHAREDMODIFIED);
                    }
                    else {
                        line->setFlags(MODIFIED);
                    }
                }
            }
            break;
        default:
            break;
    }
}

// Returns true if flush_opt
bool Cache::Snoop(ulong addr, uchar op)
{
    bool is_flush_opt = false;

    cacheLine * line = findLine(addr);

    switch(global_protocol) {
        case 0:     // MSI protocol
            if (line == NULL) /*miss*/ {
            }
            else { /*hit*/
                if (line->getFlags() == MODIFIED) {
                    flushes++;
                    writeBacks++;
                    memoryTransactions++;
                    if(op == 'r') {
                        interventions++;
                    }
                }
                if (op == 'w') {
                    line->setFlags(INVALID);
                    invalidations++;
                }
                else {
                    line->setFlags(SHARED);
                }
            }
            break;
        case 1:     // MESI protocol
            if (line == NULL) /*miss*/ {
                return is_flush_opt;
            }
            else { /*hit*/
                if (line->getFlags() == MODIFIED) {
                    if(op == 'r') {
                        interventions++;
                    }
                    flushes++;
                    writeBacks++;
                    is_flush_opt = true;
                    memoryTransactions++;
                }
                else if(line->getFlags() == EXCLUSIVE) {
                    if(op == 'r') {
                        interventions++;
                    }
                    is_flush_opt = true;
                }
                else if(line->getFlags() == SHARED) {
                    if(op != 'u') {
                        is_flush_opt = true;
                    }
                }
                if (op == 'w' || op == 'u') {
                    line->setFlags(INVALID);
                    invalidations++;
                }
                else {
                    line->setFlags(SHARED);
                }
            }
            break;
        case 2:     // Dragon protocol
            if (line == NULL) /*miss*/ {
                return is_flush_opt;
            }
            else { /*hit*/
                if (line->getFlags() == MODIFIED) {
                    line->setFlags(SHAREDMODIFIED);
                    interventions++;
                    flushes++;
                    //writeBacks++;
                    //memoryTransactions++;
                }
                else if(line->getFlags() == EXCLUSIVE) {
                    interventions++;
                    line->setFlags(SHAREDCLEAN);
                }
                else if(line->getFlags() == SHAREDMODIFIED) {
                    if(op == 'u') {
                        //memoryTransactions++;
                        line->setFlags(SHAREDCLEAN);
                    }
                    else {
                        flushes++;
                        //writeBacks++;
                        //memoryTransactions++;
                    }
                }
            }
            break;
    }
    return is_flush_opt;
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
    ulong i, j, tag, pos;

    pos = assoc;
    tag = calcTag(addr);
    i   = calcIndex(addr);

    for(j=0; j<assoc; j++)
        if(cache[i][j].isValid())
            if(cache[i][j].getTag() == tag)
            {
                pos = j; break;
            }
    if(pos == assoc)
        return NULL;
    else
        return &(cache[i][pos]);
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
    line->setSeq(currentCycle);
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
    ulong i, j, victim, min;

    victim = assoc;
    min    = currentCycle;
    i      = calcIndex(addr);

    for(j=0;j<assoc;j++)
    {
        if(cache[i][j].isValid() == 0) return &(cache[i][j]);
    }
    for(j=0;j<assoc;j++)
    {
        if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
    }
    assert(victim != assoc);

    return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
    cacheLine * victim = getLRU(addr);
    updateLRU(victim);

    return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{
    ulong tag;

    cacheLine *victim = findLineToReplace(addr);
    assert(victim != 0);
    if(victim->getFlags() == MODIFIED || victim->getFlags() == SHAREDMODIFIED) {
        writeBack(addr);
        memoryTransactions++;
    }

    tag = calcTag(addr);
    victim->setTag(tag);
    victim->setFlags(SHARED);
    /**note that this cache line has been already
       upgraded to MRU in the previous function (findLineToReplace)**/

    return victim;
}

void Cache::printStats(unsigned long processor_id)
{
    float miss_rate = float(readMisses + writeMisses)*100/float(reads + writes);

    cout    << "============ Simulation results (Cache " << processor_id << ") ============" << endl
            << "01. number of reads:\t\t\t\t" << reads << endl
            << "02. number of read misses:\t\t\t" << readMisses << endl
            << "03. number of writes:\t\t\t\t" << writes << endl
            << "04. number of write misses:\t\t\t" << writeMisses << endl
            << "05. total miss rate:\t\t\t\t" << std::fixed << std::setprecision(2) << miss_rate << "%" << endl
            << "06. number of writebacks:\t\t\t" << writeBacks << endl
            << "07. number of cache-to-cache transfers:\t\t" << transfers << endl
            << "08. number of memory transactions:\t\t" << memoryTransactions << endl
            << "09. number of interventions:\t\t\t" << interventions << endl
            << "10. number of invalidations:\t\t\t" << invalidations << endl
            << "11. number of flushes:\t\t\t\t" << flushes << endl
            << "12. number of BusRdX:\t\t\t\t" << busrdx << endl;
}
