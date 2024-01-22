	.text
	.p2align
	.globl	vfork
	.type	vfork, @function
vfork:
	.cfi_startproc
	endbr64
	sub	$8, %rsp
	call	vfork_impl@plt
	addq	$8, %rsp
	jmp	*%rax
	.cfi_endproc

	.size	vfork, .-vfork
