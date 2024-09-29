#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

.globl main
main:
        li t0, 3
        bge a0, t0, usage_ok
        la a0, usage_msg
        lw a1, 0(a1)
        call printf
        li a0, EXIT_FAILURE
        call exit
usage_ok:
        lw a0, 8(a1)
        call atoi
        call trace
        add a1, a1, 16
        lw a0, 0(a1)
        call exec
        li a0, EXIT_SUCCESS
        call exit

usage_msg: .string "usage: %s syscall_id command\n\0"


