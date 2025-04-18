# Exercise One

In this practical we are going to look at launching a simple program on the Tenstorrent device and performing some compute on one of the RISC-V baby cores

Learning objectives are:

* To understand how to transfer data to and from the Tenstorrent device
* To illustrate the role of the RISC-V baby cores and how these can access DDR and perform some compute
* To show how the host and Tenstorrent device interact with each other

>**Having problems?**
> As you go through this exercise if there is anything you are unsure about or are stuck on then you can look at the _sample_solutions_

## Getting started

Details about how to access your account on the RISC-V testbed and set up the environment that we will need for these practicals can be found on the [TODO](setup.md) page.

Irrespective of the machine you are using, it is assumed that you have a command line terminal in the _tt-tutorial/practical/one/exercise_ directory.

Once you go into this directory, type `make` and run the resulting _ex_two_ executable


```bash
user@login01:~$ make
user@login01:~$ ./ex_two
Failure on the device, 100 fails with 100 elements
```

As you can see, the host (and device) code builds, but when it launches it is not undertaking the required calculations properly, which we are going to fix now.

## Host side updates

The host side of the code, in the _ex_two.cpp_ file is fairly complete, but there have been a few omissions that need addressing. These are marked as comments with _TODO_ before them.

* Line 31: Create the buffer for the results to be written into, this will be very similar to the previous two calls
* Line 53: Copy _src1_dram_buffer_ onto the device via the `EnqueueWriteBuffer` call, you can copy how it is one the previous line (but ensure you set the buffer name to be _src1_dram_buffer_)
* Line 70: We need to pass the address of each buffer as runtime arguments to the kernel on the device. You need to replace the zero at this line with the address of your newly created results buffer (at line 31). For instance, if your results buffer is called _results_dram_buffer_ then this would be `results_dram_buffer->address()` .
* Line 82: We need to issue a `EnqueueReadBuffer` call to copy results data back from the device to the host. The arguments here are very similar to the `EnqueueWriteBuffer`, for instance `EnqueueReadBuffer(cq, results_dram_buffer, result_data, true)` . It's up to you here, you can either set _true_ for this call to block on the data transfer being undertaken, or alternatively set it to _false_ and then issue a `Finish(cq)` call to wait on completion of the command queue.

Once you have made these changes, you can then build the host file via the `make` command. Remember, this just builds the host file (the device kernels are built on demand when the executable is launched).

## Device side updates

We currently have one kernel only, a reader kernel in _kernels/data_flow/read_kernel.cpp_ that will be launched on the data in RISC-V baby core. This is fairly complete, but there is a missing part that needs provided before it will run properly.

* Line 28, we need to issue the `noc_async_read` call from the _src1_dram_noc_addr_ DRAM address into the local L1 _buffer_2_addr_ buffer. You can copy how this is done at line 25.
* Line 29, you have a choice here, we can either issue another `noc_async_read_barrier()` call after line 28 to wait for this second DRAM data read, or move the barrier from line 26 to this line. The second approach might be slightly faster, as it will but two reads onto the NoC and wait for both to complete (whereas the first approach waits for one after the other).

## Rerun

Once you have made these changes, and already rebuilt the host executable via `make`, then you can simply rerun and it should execute correctly this time:

```bash
user@login01:~$ ./ex_two
Completed successfully on the device, with 100 elements
```
