# Exercise Five

In this practical we are going to use the SFPU (vector unit) to perform the addition calculation rather than the matrix unit. As we have seen in the lecture, this is more flexible than the matrix unit, providing both a much wider range of mathematical operations but also support for comparisons. Furthermore, the vector unit more closely implements IEEE standard FP32 support so can provide improved numerical accuracy for .  

Learning objectives are:

* To understand how to drive the SFPU for maths calculations
* To illustrate that the changes needed between running on the matrix unit and vector unit are fairly minimal, providing flexibility

>**Having problems?**
> As you go through this exercise if there is anything you are unsure about or are stuck on then you can look at the [sample solutions](../sample_solutions)

## Getting started

Details about how to access your account on the RISC-V testbed and set up the environment that we will need for these practicals can be found on the [login and access instructions](https://github.com/RISCVtestbed/tt-tutorial/blob/main/practical/general/RV-testbed.md) page.

Irrespective of the machine you are using, it is assumed that you have a command line terminal in the _tt-tutorial/practical/five/exercise_ directory.

Once you go into this directory, type `make` and run the resulting _ex_five_ executable


```bash
user@login01:~$ make
user@login01:~$ ./ex_five
Failure on the device, 65536 fails with 65536 elements
```

As you can see, similarly to practical four, the host (and device) code builds, but when it launches it is not undertaking the required calculations properly. This is because the compute side of things, whilst it handles the CBs properly, requires you to add code to drive the vector unit (SFPU).

## Device side

Similarly to [practical four](../../four/exercise), we are going to be working within the _compute_kernel.cpp_ file that is in the _kernels/compute_ directory. 

Generally, the code for the vector unit and matrix engine is pretty similar, but the main difference is that the vector unit consumes from the _dst_ register and we need to explicitly copy CBs into segments of this register. Remember, the _dst_ register is 32KB in size and split up into sixteen segments, each of 2KB. The following code sketches the flow required when using the SFPU:

```c++
tile_regs_acquire();
copy_tile(source_1_cb, cb_page, dst_1_segment);
copy_tile(source_2_cb, cb_page, dst_2_segment);
sfpu_op(dst_1_segment, dst_2_segment);
tile_regs_commit();

tile_regs_wait();
pack_tile(dst_segment, target_cb);
tile_regs_release();
```

The last three lines in the above code are exactly the same as when using the matrix unit, as results are in _dst_ and need to be copied out. The first five lines of code illustrate the difference here, and as can be seen the `copy_tile` API is used to copy tiles, or chunks, of data from two CBs (_source_1_cb_ and _source_2_cb_) into different segments of the _dst_ register. The required SFPU operation is then issued, _sfpu_op_, in the above code with these two _dst_ register indexes as arguments. All SFPU operations overwrite the first source segment, so in the above example results are written to segment zero.

In the above example `sfpu_op` represents the operation to run on the vector unit, some of the most popular ones are below, with the full API available [here](https://docs.tenstorrent.com/tt-metal/latest/tt-metalium/tt_metal/apis/index.html). 

```c++
// Maths operations
add_binary_tile(....);
add_int32_tile(....);
add_uint32_tile(....);
add_uint16_tile(....);
sub_binary_tile(....);
sub_int32_tile(....);
sub_uint32_tile(....);
sub_uint16_tile(....);
mul_binary_tile(....);
mul_int32_tile(....);
mul_uint32_tile(....);
sqrt_tile(....);
square_tile(....);
sin_tile(....);
tan_tile(....);
cos_tile(....);

// Comparison operations
ltz_tile(....);
eqz_tile(....);
lez_tile(....);
gtz_tile(....);
gez_tile(....);
nez_tile(....);
```

### Compute kernel updates

Based on the above, the missing functionality in _compute_kernel.cpp_ should be fairly easy to complete:

* Line 27: Copy the tile in page 0 of the cb_in0 CB into _dst_ register segment 0
* Line 33: Copy the tile in page 0 of the cb_in1 CB into _dst_ register segment 1
* Line 39: Using the `add_binary_tile` API call add the tiles in _dst_ register segments 0 and 1

## Host code

This has remained fairly unchanged from [practical four](../../four/exercise), where the overall structure and host code are fairly static but we are then working with the compute or data movement kernels to tune and optimise them. The major difference here is that the vector unit can compute with int32, rather than limiting ourselves to int8 in [practical four](../../four/exercise).

## Rerun

Once you have made these changes, and already rebuilt the host executable via `make`, then you can simply rerun and it should execute correctly this time:

```bash
user@login01:~$ ./ex_five
Completed successfully on the device, with 65536 elements
```
