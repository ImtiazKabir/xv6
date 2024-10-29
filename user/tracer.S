#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

.globl main
main:
        li t0, 3
        blt a0, t0, wrong_usage
        lw t0, 8(a1)
        lbu t0, 0(t0)
        li t1, '0'
        blt t0, t1, wrong_usage
        li t1, '9'
        bgt t0, t1, wrong_usage
ok_usage:
        lw a0, 8(a1)
        call atoi
        call trace
        
        li t0, 0
        bge a0, t0, trace_success
trace_failure:
        lw a2, 0(a1)
        la a1, trace_msg
        li a0, 2
        call fprintf
        li a0, EXIT_FAILURE
        call exit

trace_success:
        add a1, a1, 16
        lw a0, 0(a1)
        call exec
        li a0, EXIT_SUCCESS
        call exit

wrong_usage:
        lw a2, 0(a1)
        la a1, usage_msg
        li a0, 2
        call fprintf
        li a0, EXIT_FAILURE
        call exit
        

usage_msg: .string "usage: %s syscall_id command\n\0"
trace_msg: .string "%s: trace failed\n\0"


