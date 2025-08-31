# Tenstorrent tutorial practicals

This directory contains the practicals associated with this tutorial which build upon each other:

* [Exercise one](one/exercise) gets started with running a simple algorithm on the RISC-V _baby_ data mover in core, summing pairs of numbers. It demonstrates transfering data between the host and device, launching kernels on the device and allocating buffers in L1 memory.
* [Exercise two](two/exercise) extends the first practical to perform the writing of results to DDR by the second data mover core, the RISC-V _baby_ data out core. This demonstrates Circular Buffers (CBs) which are used to communicate between the RISC-V cores in a Tensix core.
* [Exercise three](three/exercise) decouples the global problem size from the architecture by breaking the processing into chunks (or tiles). This not only means that we are not limited by the L1 buffer space that is available, but furthermore it lays the groundwork for using the compute unit to perform the calculations
* [Exercise four](four/exercise) modifies the code to use the matrix unit in the FPU to perform the addition calculation on a tile by tile basis, rather than the RISC-V baby cores doing this.
* [Exercise five](five/exercise) modifies the code to use the vector unit, SFPU, to perform the addition calculation on a tile by tile basis, rather than the matrix engine in the previous practical.

All practicals contain sample solutions, are intended to be compiled and run on EPCC's RISC-V testbed (access and login instructions [here](general/RV-testbed.md)) and leverage version 0.62.2 of the Metalium SDK.
