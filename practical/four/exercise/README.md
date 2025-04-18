# Exercise Four

In this practical we are going move the addition calculation from the RISC-V baby (data mover) into the matrix unit via the compute core. 

Learning objectives are:

* To understand how to transfer data to and from the Tenstorrent device
* To illustrate the role of the RISC-V baby cores and how these can access DDR and perform some compute
* To show how the host and Tenstorrent device interact with each other

>**Having problems?**
> As you go through this exercise if there is anything you are unsure about or are stuck on then you can look at the _sample_solutions_

## Getting started

Details about how to access your account on the RISC-V testbed and set up the environment that we will need for these practicals can be found on the [login and access instructions](https://github.com/RISCVtestbed/tt-tutorial/blob/main/practical/general/RV-testbed.md) page.

Irrespective of the machine you are using, it is assumed that you have a command line terminal in the _tt-tutorial/practical/one/exercise_ directory.

Once you go into this directory, type `make` and run the resulting _ex_one_ executable


```bash
user@login01:~$ make
user@login01:~$ ./ex_one
Failure on the device, 100 fails with 100 elements
```

As you can see, the host (and device) code builds, but when it launches it is not undertaking the required calculations properly, which we are going to fix now.

## Host side updates

The host side of the code, in the _ex_one.cpp_ file is fairly complete, but there have been a few omissions that need addressing. Before we do so, it's worth highlighting the different parts of the code to provide a picture of what is doing what here.

Between lines 21 and 40 we are allocating data buffers on the device, the first set of buffers in the DRAM memory and the second in the L1 SRAM within the specific Tensix core that we are using. All memory is initialised from a configuration, and _InterleavedBufferConfig_ is the the most common approach to use here, in this first configuration we define the overall size, the size of each page (to keep it simple we set this to be the overall size so only have one page) and the type of the buffer. Memory is then created via the `CreateBuffer` API call. The second configuration is very similar, but the flag `BufferType::L1` directs this to be allocted in the Tensix's L1 memory. We need two local areas of memory to read data into from DDR and then operate upon (we will see this in the device code a little later on).

```c++
constexpr uint32_t single_tile_size = 4 * DATA_SIZE;
InterleavedBufferConfig dram_config {
        .device = device, 
        .size = single_tile_size, 
        .page_size = single_tile_size, 
        .buffer_type = BufferType::DRAM };

std::shared_ptr<Buffer> src0_dram_buffer = CreateBuffer(dram_config);
....

InterleavedBufferConfig l1_config {
        .device= device,
        .size = single_tile_size,
        .page_size = single_tile_size,
        .buffer_type = tt::tt_metal::BufferType::L1 };

std::shared_ptr<tt::tt_metal::Buffer> l1_buffer_1 = CreateBuffer(l1_config);
std::shared_ptr<tt::tt_metal::Buffer> l1_buffer_2 = CreateBuffer(l1_config);
```

At line 52 data is transfered from the host, in _src0_data_ array to the DRAM buffer identified by the _src0_dram_buffer_ handle. Reading data back from the device (via the `EnqueueReadBuffer` API call) is very similar. The boolean here denotes whether the host should block for the data transfer to complete (_true_) or whether it should add it to the command queue and proceed (_false_). In the second case we will need to wait on the command queue completing before assuming the data has been transfered (more on this next).

```c++
EnqueueWriteBuffer(cq, src0_dram_buffer, src0_data, false);
```

We now create the kernel that will be launched on the RISC-V baby core in the Tensix unit (from filename _kernels/dataflow/read_kernel.cpp_) and then configure this with the runtime arguments which are read by the kernel when it is launched. These arguments include the addresses of the DRAM buffers, local L1 SRAM buffers and data size. The `CreateKernel` API call adds the kernel to the _program_, and this is then enqueued on the device via `EnqueueProgram`. Similarly to data transfers, the boolean argument determines whether to block or add to the command queue and continue. Lastly, the `Finish` API call will wait on the command queue to complete. It should be highlighted that items in the command queue are guaranteed to run in the order that they were added to the queue.

```c++
KernelHandle binary_reader_kernel_id = CreateKernel(
        program,
        "kernels/dataflow/read_kernel.cpp",
        core,
        DataMovementConfig{.processor = DataMovementProcessor::RISCV_0, .noc = NOC::RISCV_0_default});

SetRuntimeArgs(
        program,
        binary_reader_kernel_id,
        core,
        {src0_dram_buffer->address(),
         src1_dram_buffer->address(),
         0,
         l1_buffer_1->address(),
         l1_buffer_2->address(),
         DATA_SIZE});


EnqueueProgram(cq, program, false);   
Finish(cq);
```

Now we have looked at the major parts of the host code, let's address the small parts that are missing. These are marked as comments with _TODO_ before them.

* Line 31: Create the buffer for the results to be written into, this will be very similar to the previous two calls
* Line 53: Copy _src1_dram_buffer_ onto the device via the `EnqueueWriteBuffer` call, you can copy how it is one the previous line (but ensure you set the buffer name to be _src1_dram_buffer_)
* Line 70: We need to pass the address of each buffer as runtime arguments to the kernel on the device. You need to replace the zero at this line with the address of your newly created results buffer (at line 31). For instance, if your results buffer is called _results_dram_buffer_ then this would be `results_dram_buffer->address()` .
* Line 82: We need to issue a `EnqueueReadBuffer` call to copy results data back from the device to the host. The arguments here are very similar to the `EnqueueWriteBuffer`, for instance `EnqueueReadBuffer(cq, results_dram_buffer, result_data, true)` . It's up to you here, you can either set _true_ for this call to block on the data transfer being undertaken, or alternatively set it to _false_ and then issue a `Finish(cq)` call to wait on completion of the command queue.

Once you have made these changes, you can then build the host file via the `make` command. Remember, this just builds the host file (the device kernels are built on demand when the executable is launched).

## Device side updates

We currently have one kernel only, a reader kernel in _kernels/data_flow/read_kernel.cpp_ that will be launched on the data in RISC-V baby core. Once again, there are several parts of the code that are worth highlighting to give the reader a flavour of how they work. In the below we are retrieving the address of the _src0_dram_ buffer as a runtime argument (reading the other arguments is omitted but exactly the same, where the number refers to the argument index). This is then translated to the global address required by the Network on Chip (NoC) via the `get_noc_addr_from_bank_id` API call. Note that the first argument is zero here because this is bank ID, there are in-fact six banks of DDR memory on the board but to keep things simple we are allocating just into the first. 

```c++
uint32_t src0_dram = get_arg_val<uint32_t>(0);
....
uint64_t src0_dram_noc_addr = get_noc_addr_from_bank_id<true>(0, src0_dram);
```

The address of the first L1 SRAM buffer is passed as a runtime argument (the fourth argument, with index 3 as they start at zero). Data is then read from DDR memory via the `noc_async_read` API call, using the _src0_dram_noc_addr_ global NoC address that was already retrieved. The _bytes_data_size_ is the number of bytes that should to be read (this will be four times the data size, see the code for details). All memory accesses are non-blocking, so barriers are required before the data can be used.

```c++
uint32_t buffer_1_addr = get_arg_val<uint32_t>(3);
....
noc_async_read(src0_dram_noc_addr, buffer_1_addr, bytes_data_size);
noc_async_read_barrier();
```

Once data has been retrieved into the L1 buffers, unsigned integer pointers are then created from the L1 buffers and the core proceeds to undertake the mathematical calculation. We simply overwrite the first L1 buffer with results to avoid needing a third, L1, buffer. Lastly, these results are written out to the desination DDR memory address using `noc_async_write` API call, and once again we block for this to complete.

```c++
uint32_t * buffer_1 = (uint32_t*) buffer_1_addr;
uint32_t * buffer_2 = (uint32_t*) buffer_2_addr;

....

for (uint32_t i=0;i<data_size;i++) {
  buffer_1[i] = buffer_1[i] + buffer_2[i];
}


noc_async_write(buffer_1_addr, dst_dram_noc_addr, bytes_data_size);
noc_async_write_barrier();
```

This device code is fairly complete, but there is a missing part that needs provided before it will run properly:

* Line 28, we need to issue the `noc_async_read` call from the _src1_dram_noc_addr_ DRAM address into the local L1 _buffer_2_addr_ buffer. You can copy how this is done at line 25.
* Line 29, you have a choice here, we can either issue another `noc_async_read_barrier()` call after line 28 to wait for this second DRAM data read, or move the barrier from line 26 to this line. The second approach might be slightly faster, as it will but two reads onto the NoC and wait for both to complete (whereas the first approach waits for one after the other).

## Rerun

Once you have made these changes, and already rebuilt the host executable via `make`, then you can simply rerun and it should execute correctly this time:

```bash
user@login01:~$ ./ex_one
Completed successfully on the device, with 100 elements
```
