# Exercise Three

In this practical we are going to move closer to using the compute unit to perform the calculation. However, the matrix and vector units work on tiles of a specific size, for instance, the input registers to the matrix unit are of 4KB in size. We will extend [practical two](https://github.com/RISCVtestbed/tt-tutorial/tree/main/practical/two/exercise) to work in chunks, this decoupling of the overall size from the limits of the architecture (the compute register sizes and/or the available memory in L1) enables much larger problems to be handled. This is a fairly short practical as it is in preparation for the fourth practical where we will drive compute via the compute core.

>**Having problems?**
> As you go through this exercise if there is anything you are unsure about or are stuck on then you can look at the [sample solutions](../sample_solutions)

## Getting started

Details about how to access your account on the RISC-V testbed and set up the environment that we will need for these practicals can be found on the [login and access instructions](https://github.com/RISCVtestbed/tt-tutorial/blob/main/practical/general/RV-testbed.md) page.

Irrespective of the machine you are using, it is assumed that you have a command line terminal in the _tt-tutorial/practical/three/exercise_ directory.

## Host side updates

In the host side of the code, in the _ex_three.cpp_ file, you see that there is the addition of _CHUNK_SIZE_ and we have considerable extended the overall _DATA_SIZE_. Both kernels will run on a chunk by chunk basis, and-so we need to pass _CHUNK_SIZE_ as a runtime argument to both kernels.

* Line 66: Add _CHUNK_SIZE_ as an additional runtime argument to the _read_kernel.cpp_
* Line 94: Add _CHUNK_SIZE_ as an additional runtime argument to the _write_kernel.cpp_

Once you have made these changes, you can then build the host file via the `make` command. Remember, this just builds the host file (the device kernels are built on demand when the executable is launched).

## Device side

We are going to make broadly the same additions to the read and write kernel, threading in the _CHUNK_SIZE_ runtime argument and ensuring that data is being read from, and written to, the appropriate location in DDR based upon the chunk.

### Read kernel updates

The reader kernel, _read_kernel.cpp_ needs to be updated:

* Line 12: Define the variable _chunk_size_ as the runtime argument at index five
* Line 34 and 35: The first argument provided to the `noc_async_read` call is the DDR address, that needs to be incremented based upon the current chunk by adding the address _(i*bytes_per_chunk)_ to this.

### Write kernel updates

The reader kernel, _read_kernel.cpp_ needs to be updated:

* Line 8: Define the variable _chunk_size_ as the runtime argument at index two
* Line 29: The first argument provided to the `noc_async_write` call is the DDR address, that needs to be incremented based upon the current chunk by adding the address _(i*bytes_per_chunk)_ to this.

## Run

Once you have made these changes, and already rebuilt the host executable via `make`, then you can simply rerun and it should execute correctly this time:

```bash
user@login01:~$ ./ex_three
Completed successfully on the device, with 65536 elements
```
