# Start debugging on the kernel by exec:
    make qemu-gdb

# Connect another gdb to the server on 127.0.0.1:25501 by exec:
    > /usr/local/Cellar/riscv-gnu-toolchain/main.reinstall/bin/riscv64-unknown-elf-gdb /Users/ahmedsamir/Documents/Github/xv6-riscv/kernel/kernel
    >(gdb) target remote 127.0.0.1:25501

    >(gdb) b exec
    >(gdb) c
    

# include a new syscall to the OS (https://www.youtube.com/watch?v=hIXRrv-cBA4)
    > proc.c
    > sysproc.c
    > syscall.h
    > defs.h
    > user.h
    > usys.S
    > syscall.c
    > create a user process and include it to UPROC





# change timer interval
    > change scratch register in start.c file using syscall
    > at sysproc -> pass the parameter of the system call