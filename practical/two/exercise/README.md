# Exercise Two

In this practical we are going to look at connecting two of the RISC-V baby cores together via a Circular Buffer (CB). In [practical one](https://github.com/RISCVtestbed/tt-tutorial/tree/main/practical/one/exercise) a single RISC-V baby core (the data movement in core) read the input data, completed the calculation and write the output back to DDR. 

Here we are going to split out the writing of the results and use the second RISC-V data movement core in order to do this. This approach moves us closer to how Tenstorrent is used for real algorithms, where the data movement in core reads data, the data movement out core writes results, and the compute core drives the calculation (see practicals TODO for more on that).

Learning objectives are:

* To understand the role of the separate RISC-V data movement cores
* To illustrate how RISC-V cores communicate together
* To explore the CB API on both the host and device to show how this is used

>**Having problems?**
> As you go through this exercise if there is anything you are unsure about or are stuck on then you can look at the _sample_solutions_

## Getting started

Details about how to access your account on the RISC-V testbed and set up the environment that we will need for these practicals can be found on the [TODO](setup.md) page.

Irrespective of the machine you are using, it is assumed that you have a command line terminal in the _tt-tutorial/practical/two/exercise_ directory.

## Host side updates

The host side of the code, in the _ex_two.cpp_ file is fairly complete, but we need to add in creation of the Circular Buffer (CB) and complete configuration of the second kernel to place on the other RISC-V data movement core.

The host creates CBs on each Tensix core and a major part of this is allocating memory in the L1 SRAM. This is a little like the L1 buffer we already created in L1 for reading input data into, however CBs combine pages of memory with semantics around a producer-consumer model to control writing to and reading from CBs. 

The code below illustrates configuration in the host code for setting up the CB, where _tile_size_ is the size of a single CB tile, and _num_tiles * tile_size_ is the entire allocated memory. The data type of all CBs must also be specified, in this case an unsigned 32-bit integer.

```c++
constexpr uint32_t src0_cb_index = CBIndex::c_0;
CircularBufferConfig cb_src0_config =
  CircularBufferConfig(num_tiles * tile_size, {{src0_cb_index, tt::DataFormat::UInt32}})
    .set_page_size(src0_cb_index, tile_size);
```

* Line 45: The CB needs to be created using the configuration above, this is done via the `CreateCircularBuffer(program, core, cb_config)` call.
* Line 85: We are creating the data writer kernel and need to specify the filename of the source code, the actual file is _write_kernel.cpp_ and the path is the same as the reader kernel.
* Line 87: The _DataMovementConfig_ needs to be completed, this is the same as line 66 for the reader kernel, but on RISC-V core 1. You can capy line 66 and replace the zero with a one.

Once you have made these changes, you can then build the host file via the `make` command. Remember, this just builds the host file (the device kernels are built on demand when the executable is launched).

## Device side

There are now two kernels, _read_kernel.cpp_ and _write_kernel.cpp_, in the _kernels/dataflow_ directory. These are fairly complete, but the CB marshalling API calls need to be added.

### Reader kernel updates


## Run

Once you have made these changes, and already rebuilt the host executable via `make`, then you can simply rerun and it should execute correctly this time:

```bash
user@login01:~$ ./ex_two
Completed successfully on the device, with 100 elements
```
