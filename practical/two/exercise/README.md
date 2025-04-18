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

There are now two kernels, _read_kernel.cpp_ and _write_kernel.cpp_, in the _kernels/dataflow_ directory. These are fairly complete, but the CB marshalling API calls need to be added. There are four API calls that marshall control over the CB which are sketched below. The first two are called by the producer of data into the CB, and the second two by the consumer of data from the CB. Firstly, a certain number of pages are reserved in the CB by the producer via `cb_reserve_back`, this call will block until _number_pages_ is available and once this call has completed the producer can then work with these pages without concern around conflict. The second producer CB API call is `cb_push_back` which denotes that the producer has completed filling _number_pages_ CBs and these can then be consumed.

```c++
cb_reserve_back(cb_identifier, number_pages)
cb_push_back(cb_identifier, number_pages)

cb_wait_front(cb_identifier, number_pages)
cb_pop_front(cb_identifier, number_pages)
```

The second two API calls are called by the consumer, where `cb_wait_front` will block and wait for _number_pages_ to have been pushed by the producer. Once the data in the CB has been fully consumed, then `cb_pop_front` frees these up and makes them available again to the producer to fill in these pages with other data.

In addition to the marshalling and control, there are also two API calls highlighted below that handles the memory side. These can be used to retrieve the write pointer, `get_write_ptr`, on the producer and the read pointer, `get_read_ptr`, on the consumer. 

```c++
get_write_ptr(cb_identifier);
get_read_ptr(cb_identifier);
```

Throughout all these API calls the CB identifier, _cb_identifier_ is an argument. Ultimately this is an integer from 0 to 31 (inclusive), however the Metalium framework defines constants _tt::CBIndex::c_0_ all the way up to 31 for the naming.

### Read kernel updates

The reader kernel, _read_kernel.cpp_ needs to be updated to reserve a page in the CB and then push this once results have been written to it:

* Line 32: Using the `cb_reserve_back` API call allocate a single page in the CB
* Line 45: Using the `cb_push_back` API call push this page in the CB to the consumer

### Write kernel updates

The writer kernel, _write_kernel.cpp_ needs to be updated to wait on a page in the CB from the producer (the read kernel) and then free this up once the data has been written to DDR:

* Line 18: Using the `cb_wait_front` API call wait for a single page in the CB to be made available from the read kernel (producer)
* Line 30: Now we have written the results back to DDR, using the `cb_pop_front` API free this page up so that is can be reused by the read kernel

## Run

Once you have made these changes, and already rebuilt the host executable via `make`, then you can simply rerun and it should execute correctly this time:

```bash
user@login01:~$ ./ex_two
Completed successfully on the device, with 100 elements
```
