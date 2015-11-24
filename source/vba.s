    .file    "vba.s"
    .section .iwram,"ax",%progbits
    .code    16
    .text
    .align 2
    .global vbalog
    .thumb_func
    .type   vbalog,function
vbalog:
    mov r2,r0
    ldr r0,=0xc0ded00d
    mov r1,#0
    and r0,r0
    bx lr
    .data
