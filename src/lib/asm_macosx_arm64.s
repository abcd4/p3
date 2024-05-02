	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 12, 0	sdk_version 12, 1

	.globl	_ctx_switch                     ; -- Begin function ctx_switch
	.p2align	2
_ctx_switch:                            ; @ctx_switch
	.cfi_startproc
; %bb.0:
    sub sp, sp, #240
    stp x2, x3, [sp, #216]
    stp x4, x5, [sp, #200]
    stp x6, x7, [sp, #184]
    stp x8, x9, [sp, #168]
    stp x10, x11, [sp, #152]
    stp x12, x13, [sp, #136]
    stp x14, x15, [sp, #120]
    stp x16, x17, [sp, #104]
    stp x18, x19, [sp, #88]
    stp x20, x21, [sp, #72]
    stp x22, x23, [sp, #56]
    stp x24, x25, [sp, #40]
    stp x26, x27, [sp, #24]
    stp x28, x29, [sp, #8]
    str x30, [sp, #0]
    mov x2, sp
	str	x2, [x0]
	mov sp, x1
    ldr x30, [sp, #0]
    ldp x2, x3, [sp, #216]
    ldp x4, x5, [sp, #200]
    ldp x6, x7, [sp, #184]
    ldp x8, x9, [sp, #168]
    ldp x10, x11, [sp, #152]
    ldp x12, x13, [sp, #136]
    ldp x14, x15, [sp, #120]
    ldp x16, x17, [sp, #104]
    ldp x18, x19, [sp, #88]
    ldp x20, x21, [sp, #72]
    ldp x22, x23, [sp, #56]
    ldp x24, x25, [sp, #40]
    ldp x26, x27, [sp, #24]
    ldp x28, x29, [sp, #8]
    add sp, sp, #240
	ret
	.cfi_endproc
                                        ; -- End function

	.globl	_ctx_start                     ; -- Begin function ctx_start
	.p2align	2
_ctx_start:                            ; @ctx_start
	.cfi_startproc
; %bb.0:
    sub sp, sp, #240
    stp x2, x3, [sp, #216]
    stp x4, x5, [sp, #200]
    stp x6, x7, [sp, #184]
    stp x8, x9, [sp, #168]
    stp x10, x11, [sp, #152]
    stp x12, x13, [sp, #136]
    stp x14, x15, [sp, #120]
    stp x16, x17, [sp, #104]
    stp x18, x19, [sp, #88]
    stp x20, x21, [sp, #72]
    stp x22, x23, [sp, #56]
    stp x24, x25, [sp, #40]
    stp x26, x27, [sp, #24]
    stp x28, x29, [sp, #8]
    str x30, [sp, #0]
    mov x2, sp
    str x2, [x0]
    mov sp, x1
    bl _ctx_entry
	.cfi_endproc
                                        ; -- End function

.subsections_via_symbols
