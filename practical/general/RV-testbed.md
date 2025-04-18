# Accessing Tenstorrent

You have been provided with a visitor account on the RISC-V testbed which contains Tenstorrent accelerators. This has all the prerequisites installed and ready to be used. The username and password of your guest account will be provided to you by the tutorial team. 

## Logging into the RISC-V testbed

In order to login to the RISC-V testbed you should execute the following, where _[user-id]_ is the username that has been allocated to you. You should then enter the password when prompted. 

```bash
user@login01:~$ ssh -J [user-id]@gateway.epcc.ed.ac.uk riscv-login.epcc.ed.ac.uk
```

This will request your SSH key passphrase and the current 6-digit authentication code (TOTP) from your MFA token, which gets you access to gateway, and in addition to this you will then be asked your password for the riscv-login machine. For more details you can see [these detailed instructions](https://riscv.epcc.ed.ac.uk/documentation/getting_started/).

## Accessing the Tenstorrent machine

Once you have connected to the RISC-V testbed login node you then need to SSH to the Tenstorrent node, via issuing the following:

```bash
user@riscv-login:~$ ssh tenstorrent1
```

The API is available via the module environment to set up all the required environment variables so that you can now compile and run applications on the Tenstorrent accelerator:

```bash
user@tenstorrent1:~$ module load tt-metal/0.56
```

You can now clone this Github repository which contains the practical exercises that we will be using in this session:

```bash
user@tenstorrent1:~$ git clone https://github.com/RISCVtestbed/tt-tutorial.git
user@tenstorrent1:~$ cd tt-tutorial/practical
```
