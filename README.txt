This project is runned under Linux-4.14.39.
Need an extra system call to print the result.
The code is in hw1_syscall directory.
=====================================================
compile:
gcc hw1.c -o hw1
gcc child.c -o child
gcc hw1_FIFO.c -o hw1_FIFO
gcc hw1_RR.c -o hw1_RR
gcc hw1_SJF.c -o hw1_SJF
gcc hw1_PSJF.c -o hw1_PSJF

run:
./hw1

