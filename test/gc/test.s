	.file	"test.c"
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"key%zu"
.LC1:
	.string	"index out of range"
.LC2:
	.string	"../../include/klapi.h"
.LC3:
	.string	"assertion failed"
.LC4:
	.string	"val != NULL"
.LC5:
	.string	"expected type KL_STRING"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC6:
	.string	"klvalue_checktype(val, KL_STRING)"
	.section	.rodata.str1.1
.LC8:
	.string	"%f\n"
.LC9:
	.string	"memory used: %zu\n"
.LC10:
	.string	"gc %d times\n"
	.text
	.p2align 4
	.globl	gctest1
	.type	gctest1, @function
gctest1:
.LFB203:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	leaq	.LC0(%rip), %r14
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	xorl	%ebp, %ebp
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$56, %rsp
	.cfi_def_cfa_offset 112
	movq	%fs:40, %rax
	movq	%rax, 40(%rsp)
	xorl	%eax, %eax
	leaq	16(%rsp), %r12
	call	clock@PLT
	movq	32(%rbx), %r15
	movq	%rax, 8(%rsp)
.L7:
	movq	%r14, %rsi
	movq	%rbp, %rdx
	movq	%r12, %rdi
	xorl	%eax, %eax
	call	sprintf@PLT
	movq	32(%rbx), %rsi
	movq	56(%rbx), %rax
	cmpq	%rax, 40(%rbx)
	je	.L18
.L2:
	movq	120(%rbx), %rdi
	movq	%r12, %rsi
	call	klstrpool_new_string@PLT
	testq	%rax, %rax
	je	.L16
	movq	40(%rbx), %rdx
	leaq	16(%rdx), %rcx
	movq	%rcx, 40(%rbx)
	movq	%rax, (%rdx)
	movl	$6, 8(%rdx)
.L3:
	cmpq	48(%rbx), %rdx
	jnb	.L5
	leaq	.LC1(%rip), %r8
	leaq	.LC2(%rip), %rcx
	movl	$233, %edx
	leaq	.LC3(%rip), %rsi
	leaq	.LC4(%rip), %rdi
	call	kl_abort@PLT
	.p2align 4,,10
	.p2align 3
.L5:
	cmpl	$6, -8(%rcx)
	jne	.L19
	movq	-16(%rcx), %rsi
	movq	%rbx, %rdi
	call	klapi_storeglobal@PLT
	movq	40(%rbx), %rcx
	movl	$1, %edx
	leaq	112(%rbx), %rdi
	movq	%rcx, %rax
	subq	48(%rbx), %rax
	sarq	$4, %rax
	cmpq	$0, %rax
	cmova	%rdx, %rax
	movq	32(%rbx), %rdx
	addq	$1, %rbp
	salq	$4, %rax
	subq	%rax, %rcx
	movq	%rcx, %r13
	movq	%rcx, %rsi
	call	klreflist_close@PLT
	movq	%r13, 40(%rbx)
	cmpq	$10000000, %rbp
	jne	.L7
	call	clock@PLT
	movq	8(%rsp), %rsi
	movq	stderr(%rip), %rdi
	pxor	%xmm0, %xmm0
	leaq	.LC8(%rip), %rbp
	subq	%rsi, %rax
	movq	%rbp, %rsi
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	fprintf@PLT
	movq	56(%r15), %rdx
	movq	stderr(%rip), %rdi
	xorl	%eax, %eax
	leaq	.LC9(%rip), %rsi
	call	fprintf@PLT
	call	clock@PLT
	movq	%rbx, 40(%r15)
	movq	%rax, %r12
	movq	144(%rbx), %rax
	xorl	%ebx, %ebx
	movq	128(%rax), %rdx
	leaq	120(%rax), %rcx
	movq	$0, (%rdx)
	movq	64(%rax), %rax
	testq	%rax, %rax
	je	.L8
	.p2align 4,,10
	.p2align 3
.L9:
	movq	(%rax), %rax
	addq	$1, %rbx
	testq	%rax, %rax
	jne	.L9
.L8:
	movq	%rcx, (%rdx)
	call	clock@PLT
	pxor	%xmm0, %xmm0
	movq	stderr(%rip), %rdi
	movq	%rbp, %rsi
	subq	%r12, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	fprintf@PLT
	movq	40(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L20
	movq	stderr(%rip), %rdi
	addq	$56, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movq	%rbx, %rdx
	leaq	.LC10(%rip), %rsi
	popq	%rbx
	.cfi_def_cfa_offset 48
	xorl	%eax, %eax
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	fprintf@PLT
	.p2align 4,,10
	.p2align 3
.L18:
	.cfi_restore_state
	leaq	40(%rbx), %rdi
	call	klstack_expand@PLT
	testb	%al, %al
	jne	.L2
.L16:
	movq	40(%rbx), %rcx
	leaq	-16(%rcx), %rdx
	jmp	.L3
.L19:
	leaq	.LC5(%rip), %r8
	leaq	.LC2(%rip), %rcx
	movl	$234, %edx
	leaq	.LC3(%rip), %rsi
	leaq	.LC6(%rip), %rdi
	call	kl_abort@PLT
.L20:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE203:
	.size	gctest1, .-gctest1
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB202:
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	pxor	%xmm0, %xmm0
	subq	$80, %rsp
	.cfi_def_cfa_offset 96
	movq	%fs:40, %rax
	movq	%rax, 72(%rsp)
	xorl	%eax, %eax
	movq	%rsp, %rbx
	movaps	%xmm0, (%rsp)
	movdqa	.LC11(%rip), %xmm0
	movq	%rbx, %rdi
	movq	%rbx, 16(%rsp)
	movups	%xmm0, 56(%rsp)
	movq	%rbx, 32(%rsp)
	movq	$0, 48(%rsp)
	movq	$0, 40(%rsp)
	call	klapi_new_state@PLT
	movq	%rax, %rdi
	call	gctest1
	movq	%rbx, %rdi
	call	klmm_destroy@PLT
	movq	72(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L24
	addq	$80, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 16
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 8
	ret
.L24:
	.cfi_restore_state
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE202:
	.size	main, .-main
	.text
	.p2align 4
	.globl	gctest
	.type	gctest, @function
gctest:
.LFB204:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movq	%rdi, %r15
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	leaq	.LC0(%rip), %r13
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	leaq	112(%r15), %r12
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	xorl	%ebx, %ebx
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	leaq	32(%rsp), %rbp
	call	clock@PLT
	movq	%rax, 16(%rsp)
	movq	32(%r15), %rax
	movq	%rax, 24(%rsp)
	leaq	40(%r15), %rax
	movq	%rax, 8(%rsp)
	jmp	.L29
	.p2align 4,,10
	.p2align 3
.L26:
	movq	120(%r15), %rdi
	movq	%rbp, %rsi
	call	klstrpool_new_string@PLT
	testq	%rax, %rax
	je	.L34
	movq	40(%r15), %rsi
	leaq	16(%rsi), %rdx
	movq	%rdx, 40(%r15)
	movq	%rax, (%rsi)
	movl	$6, 8(%rsi)
.L27:
	movq	%rdx, %rax
	subq	48(%r15), %rax
	movl	$1, %esi
	movq	%r12, %rdi
	sarq	$4, %rax
	cmpq	$0, %rax
	cmova	%rsi, %rax
	addq	$1, %rbx
	salq	$4, %rax
	subq	%rax, %rdx
	movq	%rdx, %r14
	movq	32(%r15), %rdx
	movq	%r14, %rsi
	call	klreflist_close@PLT
	movq	%r14, 40(%r15)
	cmpq	$10000000, %rbx
	je	.L35
.L29:
	movq	%r13, %rsi
	movq	%rbx, %rdx
	movq	%rbp, %rdi
	xorl	%eax, %eax
	call	sprintf@PLT
	movq	32(%r15), %rsi
	movq	56(%r15), %rax
	cmpq	%rax, 40(%r15)
	jne	.L26
	movq	8(%rsp), %rdi
	call	klstack_expand@PLT
	testb	%al, %al
	jne	.L26
.L34:
	movq	40(%r15), %rdx
	jmp	.L27
.L35:
	call	clock@PLT
	movq	16(%rsp), %rcx
	movq	stderr(%rip), %rdi
	leaq	.LC8(%rip), %rbp
	pxor	%xmm0, %xmm0
	movq	%rbp, %rsi
	subq	%rcx, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	fprintf@PLT
	movq	24(%rsp), %r14
	movq	stderr(%rip), %rdi
	xorl	%eax, %eax
	leaq	.LC9(%rip), %rsi
	movq	56(%r14), %rdx
	call	fprintf@PLT
	call	clock@PLT
	movq	%r15, 40(%r14)
	movq	%r14, %rdi
	movq	%rax, %rbx
	call	klmm_do_gc@PLT
	call	clock@PLT
	pxor	%xmm0, %xmm0
	movq	stderr(%rip), %rdi
	movq	%rbp, %rsi
	subq	%rbx, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	fprintf@PLT
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L36
	movl	a(%rip), %edx
	movq	stderr(%rip), %rdi
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	leaq	.LC10(%rip), %rsi
	popq	%rbx
	.cfi_def_cfa_offset 48
	xorl	%eax, %eax
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	fprintf@PLT
.L36:
	.cfi_restore_state
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE204:
	.size	gctest, .-gctest
	.section	.rodata.str1.1
.LC13:
	.string	"name"
.LC17:
	.string	"%s\n"
	.section	.text.unlikely,"ax",@progbits
.LCOLDB18:
	.text
.LHOTB18:
	.p2align 4
	.globl	gctest0
	.type	gctest0, @function
gctest0:
.LFB205:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx
	subq	$24, %rsp
	.cfi_def_cfa_offset 80
	call	clock@PLT
	movq	32(%rbx), %rbp
	movq	%rax, %r14
	movq	64(%rbp), %rax
	cmpq	%rax, 56(%rbp)
	jnb	.L63
.L38:
	movl	$400, %edi
	call	malloc@PLT
	movq	%rax, %r12
	testq	%rax, %rax
	je	.L39
.L62:
	addq	$400, 56(%rbp)
.L40:
	pushq	$0
	.cfi_def_cfa_offset 88
	movl	$2, %ecx
	movl	$100, %edx
	movq	%r12, %rsi
	pushq	$100
	.cfi_def_cfa_offset 96
	movq	%rbp, %rdi
	xorl	%r9d, %r9d
	xorl	%r8d, %r8d
	call	klkfunc_alloc@PLT
	popq	%rcx
	.cfi_def_cfa_offset 88
	movq	32(%rbx), %rsi
	movq	40(%rax), %rdx
	movq	%rax, %r13
	popq	%rdi
	.cfi_def_cfa_offset 80
	movq	40(%rbx), %rax
	cmpq	56(%rbx), %rax
	je	.L64
.L42:
	leaq	16(%rax), %r15
	movq	%r15, 40(%rbx)
	movq	$10000000, (%rax)
	movl	$0, 8(%rax)
.L44:
	cmpq	48(%rbx), %rax
	jb	.L60
	movdqu	-16(%r15), %xmm1
	movq	120(%rbx), %rdi
	movq	%rdx, 8(%rsp)
	leaq	.LC13(%rip), %rsi
	movups	%xmm1, (%rdx)
	call	klstrpool_new_string@PLT
	movq	8(%rsp), %rdx
	testq	%rax, %rax
	je	.L46
	movq	%rax, -16(%r15)
	movl	$6, -8(%r15)
.L46:
	movq	40(%rbx), %rcx
	leaq	-16(%rcx), %rax
	cmpq	48(%rbx), %rax
	jb	.L47
	movdqu	-16(%rcx), %xmm2
	xorl	%r8d, %r8d
	leaq	112(%rbx), %rcx
	movq	%r13, %rsi
	movdqa	.LC14(%rip), %xmm0
	movq	%rbp, %rdi
	movups	%xmm2, 16(%rdx)
	movq	.LC16(%rip), %rdx
	movups	%xmm0, (%r12)
	movdqa	.LC15(%rip), %xmm0
	movq	%rdx, 32(%r12)
	movq	%rax, %rdx
	movups	%xmm0, 16(%r12)
	call	klkclosure_create@PLT
	movq	%r13, %rsi
	movq	%rbp, %rdi
	movq	%rax, %r12
	call	klkfunc_initdone@PLT
	movq	40(%rbx), %rdx
	leaq	-16(%rdx), %rsi
	cmpq	48(%rbx), %rsi
	jb	.L50
	movq	%r12, -16(%rdx)
	movl	$11, -8(%rdx)
.L48:
	movl	$1, %edx
	movl	$1, %ecx
	movq	%rbx, %rdi
	call	klexec_call@PLT
	leaq	.LC8(%rip), %r12
	movl	%eax, %r13d
	call	clock@PLT
	movq	stderr(%rip), %rdi
	pxor	%xmm0, %xmm0
	movq	%r12, %rsi
	subq	%r14, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	fprintf@PLT
	movq	56(%rbp), %rdx
	xorl	%eax, %eax
	movq	stderr(%rip), %rdi
	leaq	.LC9(%rip), %rsi
	call	fprintf@PLT
	testl	%r13d, %r13d
	jne	.L65
.L49:
	movq	%rbx, 40(%rbp)
	call	clock@PLT
	movq	%rbp, %rdi
	movq	%rax, %rbx
	call	klmm_do_gc@PLT
	call	clock@PLT
	movq	stderr(%rip), %rdi
	pxor	%xmm0, %xmm0
	movq	%r12, %rsi
	subq	%rbx, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	fprintf@PLT
	movl	a(%rip), %edx
	movq	stderr(%rip), %rdi
	xorl	%eax, %eax
	addq	$24, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	leaq	.LC10(%rip), %rsi
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	jmp	fprintf@PLT
	.p2align 4,,10
	.p2align 3
.L50:
	.cfi_restore_state
	xorl	%esi, %esi
	jmp	.L48
	.p2align 4,,10
	.p2align 3
.L63:
	movq	%rbp, %rdi
	call	klmm_do_gc@PLT
	jmp	.L38
	.p2align 4,,10
	.p2align 3
.L65:
	movq	96(%rbx), %rdx
	movq	stderr(%rip), %rdi
	leaq	.LC17(%rip), %rsi
	xorl	%eax, %eax
	call	fprintf@PLT
	jmp	.L49
	.p2align 4,,10
	.p2align 3
.L64:
	leaq	40(%rbx), %rdi
	movq	%rdx, 8(%rsp)
	call	klstack_expand@PLT
	movq	8(%rsp), %rdx
	testb	%al, %al
	je	.L66
	movq	40(%rbx), %rax
	jmp	.L42
.L66:
	movq	40(%rbx), %r15
	leaq	-16(%r15), %rax
	jmp	.L44
.L39:
	movq	%rbp, %rdi
	call	klmm_do_gc@PLT
	movl	$400, %edi
	call	malloc@PLT
	movq	%rax, %r12
	testq	%rax, %rax
	jne	.L62
	jmp	.L40
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.type	gctest0.cold, @function
gctest0.cold:
.LFSB205:
.L60:
	.cfi_def_cfa_offset 80
	.cfi_offset 3, -56
	.cfi_offset 6, -48
	.cfi_offset 12, -40
	.cfi_offset 13, -32
	.cfi_offset 14, -24
	.cfi_offset 15, -16
	movdqa	0, %xmm0
	movups	%xmm0, (%rdx)
	ud2
.L47:
	movdqa	0, %xmm0
	movups	%xmm0, 16(%rdx)
	ud2
	.cfi_endproc
.LFE205:
	.text
	.size	gctest0, .-gctest0
	.section	.text.unlikely
	.size	gctest0.cold, .-gctest0.cold
.LCOLDE18:
	.text
.LHOTE18:
	.globl	a
	.bss
	.align 4
	.type	a, @object
	.size	a, 4
a:
	.zero	4
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC7:
	.long	1232348160
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC11:
	.quad	0
	.quad	1024
	.align 16
.LC14:
	.long	93
	.long	-2147483619
	.long	286
	.long	543
	.align 16
.LC15:
	.long	-2147221408
	.long	197412
	.long	66590
	.long	67306541
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC16:
	.long	2147221601
	.long	27
	.ident	"GCC: (GNU) 13.2.1 20230801"
	.section	.note.GNU-stack,"",@progbits
