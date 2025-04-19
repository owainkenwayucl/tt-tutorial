# Exercise Four

In this practical we are going move the addition calculation from the RISC-V baby (data mover) into the matrix unit via the compute core. 

Learning objectives are:

* To understand how the compute unit operates and how one interacts with it
* To illustrate the structure of an algorithm that properly takes advantage of the compute capabilities of the Tensix
* To show how to drive the matrix engine and use this for undertaking arithmetic operations

>**Having problems?**
> As you go through this exercise if there is anything you are unsure about or are stuck on then you can look at the [sample solutions](../sample_solutions)

## Getting started

Details about how to access your account on the RISC-V testbed and set up the environment that we will need for these practicals can be found on the [login and access instructions](https://github.com/RISCVtestbed/tt-tutorial/blob/main/practical/general/RV-testbed.md) page.

Irrespective of the machine you are using, it is assumed that you have a command line terminal in the _tt-tutorial/practical/four/exercise_ directory.

Once you go into this directory, type `make` and run the resulting _ex_four_ executable


```bash
user@login01:~$ make
user@login01:~$ ./ex_four
Failure on the device, 65536 fails with 65536 elements
```

As you can see a little bit like practical one, the host (and device) code builds, but when it launches it is not undertaking the required calculations properly. This is because the compute side of things, whilst it handles the CBs properly, requires you to add code to drive the matrix engine.

## Device side

If you look in the _kernels_ directory you will see an additional _compute_ folder, and in there the compute kernel which is named _compute_kernel.cpp_ . It is this kernel that we are going to focus on here.

There are quite a few things that needed to be added to this file (marked with comments), and broadly we need to handle the destimation register (_dst_) locks, instruct the matrix engine to operate upon the CBs, and then retrieve result data from the destimation register, _dst_, into an output CB. It is important to highlight that generally compute unit API calls operate on CBs as inputs and outputs.

There are four API calls that manage locks around the destination register. The reason we need these locks is that the compute kernel is actually run by three RISC-V baby cores; the unpacker, the maths core and the packer. As the name suggests, the unpacker will instruct the compute unit to unpack data from CBs into the compute unit's input registers (_srcA_ and _srcB_), the maths core then instructs the matrix unit (or vector unit) to undertake a specific operation on the input registers, and the packer core will then instruct the compute unit to copy data from the _dst_ registers to a CB as an output.

The structure of compute is illustrated below, where the math core aquires the dst tile registers via `tile_regs_acquire`, the matrix unit operation is then performed on two CBs as input, with the page indexes of each of these CBs provided as arguments three and four. Lastly, the segment in the _dst_ register to place results is also provided (there are 16 segments, each 2KB in size). Next the `tile_regs_commit` call releases the registers and denotes that these can be obtained by the packer.

```c++
tile_regs_acquire();
mm_op(cb_1_index, cb_2_index, cb_1_page, cb_2_page, dst_segment);
tile_regs_commit();

tile_regs_wait();
pack_tile(dst_segment, target_cb);
tile_regs_release();
```

The last three lines in the snippet above will wait for tile registers on the packer core, once these are obtained the packer will then instruct the compute unit to pack the CB via the `pack_tile` API call, with the segment in the _dst_ register as the first argument (this should the same as provided to the _mm_op_ API call). Lastly, tile registers are released via `tile_regs_release`.

In the above example `mm_op` represents the operation to run on the matrix unit, the most popular ones are below, with the full API available [here](https://docs.tenstorrent.com/tt-metal/latest/tt-metalium/tt_metal/apis/index.html). 

```c++
add_tiles(....);
sub_tiles(....);
mul_tiles(....);
matmul_tiles(....);
transpose_wh_tile(....);
reduce_tile(....);
```

### Compute kernel updates

Based on the above, the missing functionality in _compute_kernel.cpp_ should be fairly easy to complete:

* Line 23: Aquire dst tile registers via `tile_regs_acquire`
* Line 25: Execute the `add_tiles` operation on the matrix unit
* Line 31: Commit the dst registers via `tile_regs_commit`
* Line 40: Wait for the dst tile registers via `tile_regs_wait`
* Line 42: Pack results in dst tile registers into the _cb_out0_ CB via `pack_tile`
* Line 46: Release dst tile registers

## Host code

The host code is complete, but it's worth having a look at the differences here. We no longer create the L1 buffers because, instead, we are creating three CBs (line 30 to 44 in _ex_four.cpp_) and the first data movement (in) core will read directly into these from DDR and then push to make them available to the compute core. You can also see between lines 92 and 110 the creation and configuration of the compute kernel. This is illustrated in the code snippet below, where setting the runtime arguments is the same as before, however creation of the kernel is different as a _ComputeConfig_ must be provided which configures the compute unit.

```c++
// Compute kernel creation
KernelHandle compute_kernel_id = CreateKernel(
  program,
  "kernels/compute/compute_kernel.cpp",
  core,
  ComputeConfig{
    .math_fidelity = MathFidelity::HiFi4,
    .fp32_dest_acc_en = false,
    .math_approx_mode = false,
    .compile_args = {},
});

// Configure compute runtime kernel arguments
SetRuntimeArgs(
  program,
  compute_kernel_id,
  core,
  {DATA_SIZE,
  CHUNK_SIZE});
```

## Rerun

Once you have made these changes, and already rebuilt the host executable via `make`, then you can simply rerun and it should execute correctly this time:

```bash
user@login01:~$ ./ex_four
Completed successfully on the device, with 65536 elements
```
