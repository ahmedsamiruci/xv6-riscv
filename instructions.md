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



// append to the file
make qemu | ts '[%H:%M:%.S]' | tee -a ./testtime.txt

// create/replace file
make qemu | ts '[%H:%M:%.S]' | tee ./parsing.txt

make qemu | ts '[%H:%M:%.S]' | tee ./testqueu.txt

test2 &;test3 &;test4 &;test5 &;test6 &;



53      run     pcb     0       0
52      runble  test6   0       3
45      runble  test2   36      7
47      runble  test3   0       6
49      runble  test4   0       5
51      runble  test5   0       4

test4 = 21 ticks
test2 = 36 ticks
test3 = 64 ticks
test6 = 94 ticks
test5 = 121 ticks