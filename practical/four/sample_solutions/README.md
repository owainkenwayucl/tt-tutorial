# Exercise Four: Sample solutions

In this practical we move the addition calculation from the RISC-V baby (data mover) into the matrix unit via the compute core.  

## Building and running

Details about how to access your account on the RISC-V testbed and set up the environment that we will need for these practicals can be found on the [login and access instructions](https://github.com/RISCVtestbed/tt-tutorial/blob/main/practical/general/RV-testbed.md) page.

Irrespective of the machine you are using, it is assumed that you have a command line terminal in the _tt-tutorial/practical/four/exercise_ directory.

Once you go into this directory, type `make` and run the resulting _ex_four_ executable

```bash
user@login01:~$ make
user@login01:~$ ./ex_four
Completed successfully on the device, with 65536 elements
```
