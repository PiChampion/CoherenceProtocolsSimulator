# Coherence Protocols Simulator

The project was completed as part of ECE 506 (Architecture of Parallel Computers) at NC State. In this project, a trace-driven SMP simulator (shared multiprocessor simulator) was constructed starting from a C++ generic‐cache class. This was extended to work in a multi-processor environment and to build various coherence protocols on top of it. The purpose of this project is to give an idea of how parallel architecture designs are evaluated, and how to interpret performance data.

A make file Makefilegenerates the executable binary. In this project, one level of the cache is implemented only to maintain coherence across one level of cache. For simplicity, it is assumed that each processor has only one private L1 cache connected to the main memory directly through a shared bus. MSI, MESI, and Dragon coherence protocols were implemented in this project.

The simulator is able to specify aspects of the systems such as cache size, associativity, block size, number of processors, and the protocol being used.

### Memory Hierarchy Specifications:
* Replacement Policy: LRU
* Write policy: write-back + write-allocate

### The simulator outputs the following statistics:
* Memory hierarchy configuration and trace filename.
* The following measurements:
  * Number of read transactions the cache has received
  * Number of read misses the cache has suffered
  * Number of write transactions the cache has received
  * Number of write misses the cache has suffered
  * Total miss rate
  * Number of dirty blocks written back to the main memory
  * Number of cache-to-cache transfers
  * Number of memory transactions
  * Number of interventions
  * Number of invalidations
  * Number of flushes to the main memory
  * Number of issued BusRdX transactions
