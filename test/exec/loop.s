	.file	"loop.c"
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"%zu\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB22:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movq	8(%rsi), %rdi
	movl	$10, %edx
	xorl	%esi, %esi
	call	strtoull@PLT
	xorl	%esi, %esi
	testq	%rax, %rax
	je	.L2
	xorl	%edx, %edx
	testb	$1, %al
	je	.L3
	movl	$1, %edx
	cmpq	$1, %rax
	je	.L2
	.p2align 4,,10
	.p2align 3
.L3:
	leaq	1(%rsi,%rdx,2), %rsi
	addq	$2, %rdx
	cmpq	%rdx, %rax
	jne	.L3
.L2:
	leaq	.LC0(%rip), %rdi
	xorl	%eax, %eax
	call	printf@PLT
	xorl	%eax, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE22:
	.size	main, .-main
	.ident	"GCC: (GNU) 13.2.1 20230801"
	.section	.note.GNU-stack,"",@progbits
