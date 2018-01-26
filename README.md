HTM Skip Lists
===============

Concurrent skip list implementations (dictionaries and priority queues) using Intel's Restricted Transactional Memory (RTM) interface. 

The project also includes non-HTM implementations for comparison reasons. The lock-based, lock-free and sequential dictionaries are derived from ASCYLIB (https://github.com/LPD-EPFL/ASCYLIB) and the SprayList is derived from the original source (https://github.com/jkopinsky/SprayList).

This is a project that I developed for my [thesis](./thesis.pdf) at ECE-NTUA.

Algorithms
----------

The following table contains the algorithms included in this project:

| # |    Name   |  Progress  |  Year | Referece |
|:-:|-----------|:-----:|:-----:|:-----:|
|| **Dictionaries** ||||
|1| [Sequential skip list](./src/skiplist-seq/) |	sequential | | |
|2| [Herlihy et al. skip list](./src/skiplist-herlihy_lb/) |	lock-based | 2007 | [[HLL+07]](#HLL+07) |
|3| [Fraser skip list with Herlihy's optimization](./src/skiplist-herlihy_lf/) |	lock-free | 2011 | [[HLS+11]](#HLS+11) |
|4| [HTM skip list](./src/skiplist-htm/) |	htm-based | 2017 | |
|| **Priority Queues** ||||
|5| [Alistarh et al. priority queue](./src/spraylist/) |	lock-free | 2015 | [[AKL+15]](#AKL+15) | 
|6| [HTM spray list](./src/spraylist-htm/) |	htm-based | 2017 | |


References
----------

* <a name="AKL+15">**[AKL+15]**</a>
D. Alistarh, J. Kopinsky, J. Li, N. Shavit. The SprayList:
*A Scalable Relaxed Priority Queue*. 
PPoPP '15.
* <a name="HLL+07">**[HLL+07]**</a>
M. Herlihy, Y. Lev, V. Luchangco, and N. Shavit.
*A Simple Optimistic Skiplist Algorithm*.
SIROCCO '07.
* <a name="HLS+11">**[HLS+11]**</a>
M. Herlihy, Y. Lev, and N. Shavit.
*Concurrent lock-free skiplist with wait-free contains operator*, May 3 2011.
US Patent 7,937,378.

Compilation
-----------

The following instructions are derived from ASCYLIB. They occur here similarly.

HTM Skip Lists requires the ssmem memory allocator (https://github.com/LPD-EPFL/ssmem).
ssmem is already compiled and included in external/lib for x86_64, SPARC, and the Tilera architectures.
In order to create your own build of ssmem, take the following steps.
Clone ssmem, do `make libssmem.a` and then copy `libssmem.a` in `HTM_Skip-Lists/external/lib` and `smmem.h` in `HTM_Skip-Lists/external/include`.

Additionally, the sspfd profiler library is required (https://github.com/trigonak/sspfd).
sspfd is already compiled and included in external/lib for x86_64, SPARC, and the Tilera architectures.
In order to create your own build of sspfd, take the following steps.
Clone sspfd, do `make` and then copy `libsspfd.a` in `HTM_Skip-Lists/external/lib` and `sspfd.h` in `HTM_Skip-Lists/external/include`.

Finally, to measure power on new Intel processors (e.g., Intel Ivy Bridge), the raplread library is required (https://github.com/LPD-EPFL/raplread).
raplread is already compiled and included in external/lib.
In order to create your own build of raplread, take the following steps.
Clone raplread, do `make` and then copy `libraplread.a` in `HTM_Skip-Lists/external/lib` and `sspfd.h` in `HTM_Skip-Lists/external/include`.
The current version of HTM Skip Lists doesn't support power measurement

To build all data structures, you can execute `make all` or `make`.
This target builds all lock-free, lock-based, and sequential data structures.

HTM Skip Lists includes a default configuration that uses `gcc` and tries to infer the number of cores and the frequency of the target/build platform. If this configuration is incorrect, you can always create a manual configurations in `common/Makefile.common` and `include/utils.h` (look in these files for examples). 

HTM Skip Lists accepts various compilation parameters. Please refer to the `COMPILE` file.

Tests
-----

Building HTM Skip Lists generates per-data-structure benchmarks in the `bin` directory.
Issue `./bin/executable -h` for the parameters each of those accepts.

Depending on the compilation flags, these benchmarks can be set to measure throughtput, latency, and/or power-consumption statistics. (currently supporting only throughput)

Scripts
-------

Scripts used for various measurements can be found in the `scripts` folder. All the ASCYLIB scripts are in `scripts/ASCYLIB_scripts` .

Acknowledgments
----------

I would like to give special thanks to PhD candidate Dimitrios Siakavaras for his valuable help and guidance during the development of this project.

