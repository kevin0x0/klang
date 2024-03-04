	.file	"test.c"
	.text
	.p2align 4
	.globl	expect
	.type	expect, @function
expect:
.LFB180:
	.cfi_startproc
	cmpq	$100, %rdi
	ja	.L4
	addq	$2, %rdi
	jmp	malloc@PLT
	.p2align 4,,10
	.p2align 3
.L4:
	imulq	%rdi, %rdi
	jmp	malloc@PLT
	.cfi_endproc
.LFE180:
	.size	expect, .-expect
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"fibonacci"
	.section	.text.unlikely,"ax",@progbits
.LCOLDB4:
	.text
.LHOTB4:
	.p2align 4
	.globl	fibonacci
	.type	fibonacci, @function
fibonacci:
.LFB182:
	.cfi_startproc
	pushq	%r14
	.cfi_def_cfa_offset 16
	.cfi_offset 14, -16
	pushq	%r13
	.cfi_def_cfa_offset 24
	.cfi_offset 13, -24
	pushq	%r12
	.cfi_def_cfa_offset 32
	.cfi_offset 12, -32
	pushq	%rbp
	.cfi_def_cfa_offset 40
	.cfi_offset 6, -40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	movq	16(%rdi), %r12
	movq	%rdi, %rbx
	movq	%r12, %rdi
	call	klmm_do_gc@PLT
	movl	$400, %edi
	call	malloc@PLT
	testq	%rax, %rax
	je	.L6
	addq	$400, 56(%r12)
	movq	%rax, %rbp
.L7:
	subq	$8, %rsp
	.cfi_def_cfa_offset 56
	movl	$100, %edx
	movq	%rbp, %rsi
	xorl	%r8d, %r8d
	pushq	$1
	.cfi_def_cfa_offset 64
	movl	$100, %r9d
	movl	$1, %ecx
	movq	%r12, %rdi
	call	klkfunc_alloc@PLT
	movq	16(%rbx), %rsi
	movq	48(%rax), %r14
	movq	%rax, %r13
	popq	%rax
	.cfi_def_cfa_offset 56
	movq	56(%rbx), %rax
	popq	%rdx
	.cfi_def_cfa_offset 48
	cmpq	%rax, 40(%rbx)
	je	.L19
.L9:
	movq	120(%rbx), %rdi
	leaq	.LC0(%rip), %rsi
	call	klstrpool_new_string@PLT
	testq	%rax, %rax
	je	.L18
	movq	40(%rbx), %rdx
	leaq	16(%rdx), %rcx
	movq	%rcx, 40(%rbx)
	movq	%rax, (%rdx)
	movl	$5, 8(%rdx)
.L10:
	cmpq	48(%rbx), %rdx
	jb	.L12
	movdqu	(%rdx), %xmm1
	leaq	112(%rbx), %rcx
	xorl	%r8d, %r8d
	movq	%r13, %rsi
	movdqa	.LC1(%rip), %xmm0
	movq	.LC3(%rip), %rax
	movq	%r12, %rdi
	movups	%xmm1, (%r14)
	movups	%xmm0, 0(%rbp)
	movdqa	.LC2(%rip), %xmm0
	movq	%rax, 32(%rbp)
	movl	$278, 40(%rbp)
	movups	%xmm0, 16(%rbp)
	call	klkclosure_create@PLT
	movq	%r13, %rsi
	movq	%r12, %rdi
	movq	%rax, %rbp
	call	klkfunc_initdone@PLT
	movq	40(%rbx), %rdx
	leaq	-16(%rdx), %rax
	cmpq	48(%rbx), %rax
	jb	.L13
	movq	%rbp, -16(%rdx)
	movl	$10, -8(%rdx)
.L13:
	movq	120(%rbx), %rdi
	leaq	.LC0(%rip), %rsi
	call	klstrpool_new_string@PLT
	movq	%rbx, %rdi
	popq	%rbx
	.cfi_remember_state
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_def_cfa_offset 32
	movq	%rax, %rsi
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	jmp	klapi_storeglobal@PLT
	.p2align 4,,10
	.p2align 3
.L19:
	.cfi_restore_state
	leaq	40(%rbx), %rdi
	call	klstack_expand@PLT
	testb	%al, %al
	jne	.L9
.L18:
	movq	40(%rbx), %rax
	leaq	-16(%rax), %rdx
	jmp	.L10
.L6:
	movq	%r12, %rdi
	call	klmm_do_gc@PLT
	movl	$400, %edi
	call	malloc@PLT
	movq	%rax, %rbp
	testq	%rax, %rax
	je	.L7
	addq	$400, 56(%r12)
	jmp	.L7
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.type	fibonacci.cold, @function
fibonacci.cold:
.LFSB182:
.L12:
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	.cfi_offset 6, -40
	.cfi_offset 12, -32
	.cfi_offset 13, -24
	.cfi_offset 14, -16
	movdqa	0, %xmm0
	movups	%xmm0, (%r14)
	ud2
	.cfi_endproc
.LFE182:
	.text
	.size	fibonacci, .-fibonacci
	.section	.text.unlikely
	.size	fibonacci.cold, .-fibonacci.cold
.LCOLDE4:
	.text
.LHOTE4:
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC6:
	.string	"out of memory when calling a callable object"
	.section	.rodata.str1.1
.LC8:
	.string	"%f\n"
.LC9:
	.string	"%s\n"
.LC10:
	.string	"fibonacci(%d) = %zd\n"
	.section	.text.unlikely
.LCOLDB12:
	.section	.text.startup,"ax",@progbits
.LHOTB12:
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB181:
	.cfi_startproc
	pushq	%r13
	.cfi_def_cfa_offset 16
	.cfi_offset 13, -16
	pxor	%xmm0, %xmm0
	pushq	%r12
	.cfi_def_cfa_offset 24
	.cfi_offset 12, -24
	pushq	%rbp
	.cfi_def_cfa_offset 32
	.cfi_offset 6, -32
	pushq	%rbx
	.cfi_def_cfa_offset 40
	.cfi_offset 3, -40
	subq	$104, %rsp
	.cfi_def_cfa_offset 144
	movq	%fs:40, %rax
	movq	%rax, 88(%rsp)
	xorl	%eax, %eax
	leaq	16(%rsp), %rbp
	movaps	%xmm0, 16(%rsp)
	movdqa	.LC5(%rip), %xmm0
	movq	%rbp, %rdi
	movq	%rbp, 32(%rsp)
	movups	%xmm0, 72(%rsp)
	movq	%rbp, 56(%rsp)
	movq	$0, 64(%rsp)
	call	klapi_new_state@PLT
	movq	%rax, %rbx
	movq	%rax, %rdi
	call	fibonacci
	movq	16(%rbx), %rsi
	movq	40(%rbx), %rax
	cmpq	56(%rbx), %rax
	je	.L44
.L21:
	leaq	16(%rax), %rdx
	movq	%rdx, 40(%rbx)
	movq	$35, (%rax)
	movl	$1, 8(%rax)
.L22:
	call	clock@PLT
	movq	%rax, %r12
	movq	40(%rbx), %rax
	leaq	-32(%rax), %rdx
	cmpq	48(%rbx), %rdx
	jnb	.L23
	xorl	%edx, %edx
.L23:
	movq	136(%rbx), %r13
	movq	8(%r13), %rsi
	testq	%rsi, %rsi
	je	.L45
.L24:
	xorl	%eax, %eax
	movb	$1, 56(%rsi)
	movl	$1, %ecx
	movq	%rbx, %rdi
	movw	%ax, 58(%rsi)
	movq	$0, 16(%rsi)
	movl	$0, 24(%rsi)
	call	klexec_callprepare@PLT
	testl	%eax, %eax
	jne	.L46
	movq	136(%rbx), %rax
	cmpq	%rax, %r13
	je	.L29
	orl	$1, 60(%rax)
	movq	%rbx, %rdi
	call	klexec_execute@PLT
	movl	%eax, %r13d
.L28:
	call	clock@PLT
	pxor	%xmm0, %xmm0
	leaq	.LC8(%rip), %rdi
	subq	%r12, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	printf@PLT
	testl	%r13d, %r13d
	jne	.L33
.L30:
	movq	40(%rbx), %rax
	leaq	-16(%rax), %rdx
	cmpq	48(%rbx), %rdx
	jb	.L32
	movq	-16(%rax), %rdx
	leaq	.LC10(%rip), %rdi
	movl	$35, %esi
	xorl	%eax, %eax
	call	printf@PLT
	movq	%rbp, %rdi
	call	klmm_destroy@PLT
.L31:
	movq	88(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L47
	addq	$104, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 40
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 32
	popq	%rbp
	.cfi_def_cfa_offset 24
	popq	%r12
	.cfi_def_cfa_offset 16
	popq	%r13
	.cfi_def_cfa_offset 8
	ret
.L46:
	.cfi_restore_state
	call	clock@PLT
	pxor	%xmm0, %xmm0
	leaq	.LC8(%rip), %rdi
	subq	%r12, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	printf@PLT
.L33:
	movq	96(%rbx), %rdx
	movq	stderr(%rip), %rdi
	leaq	.LC9(%rip), %rsi
	xorl	%eax, %eax
	call	fprintf@PLT
	jmp	.L31
.L45:
	movq	%rbx, %rdi
	movq	%rdx, 8(%rsp)
	call	klexec_alloc_callinfo@PLT
	movq	8(%rsp), %rdx
	testq	%rax, %rax
	movq	%rax, %rsi
	jne	.L24
	leaq	.LC6(%rip), %rdx
	orl	$-1, %esi
	movq	%rbx, %rdi
	xorl	%eax, %eax
	call	klstate_throw@PLT
	movl	%eax, %r13d
	jmp	.L28
.L44:
	leaq	40(%rbx), %rdi
	call	klstack_expand@PLT
	testb	%al, %al
	je	.L22
	movq	40(%rbx), %rax
	jmp	.L21
.L29:
	call	clock@PLT
	pxor	%xmm0, %xmm0
	leaq	.LC8(%rip), %rdi
	subq	%r12, %rax
	cvtsi2ssq	%rax, %xmm0
	movl	$1, %eax
	divss	.LC7(%rip), %xmm0
	cvtss2sd	%xmm0, %xmm0
	call	printf@PLT
	jmp	.L30
.L47:
	call	__stack_chk_fail@PLT
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.type	main.cold, @function
main.cold:
.LFSB181:
.L32:
	.cfi_def_cfa_offset 144
	.cfi_offset 3, -40
	.cfi_offset 6, -32
	.cfi_offset 12, -24
	.cfi_offset 13, -16
	movq	0, %rax
	ud2
	.cfi_endproc
.LFE181:
	.section	.text.startup
	.size	main, .-main
	.section	.text.unlikely
	.size	main.cold, .-main.cold
.LCOLDE12:
	.section	.text.startup
.LHOTE12:
	.section	.rodata.str1.1
.LC13:
	.string	""
	.section	.text.unlikely
.LCOLDB16:
	.text
.LHOTB16:
	.p2align 4
	.globl	concat
	.type	concat, @function
concat:
.LFB183:
	.cfi_startproc
	pushq	%r14
	.cfi_def_cfa_offset 16
	.cfi_offset 14, -16
	pushq	%r13
	.cfi_def_cfa_offset 24
	.cfi_offset 13, -24
	pushq	%r12
	.cfi_def_cfa_offset 32
	.cfi_offset 12, -32
	pushq	%rbp
	.cfi_def_cfa_offset 40
	.cfi_offset 6, -40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	movq	16(%rdi), %r12
	movq	%rdi, %rbx
	movq	%r12, %rdi
	call	klmm_do_gc@PLT
	movl	$400, %edi
	call	malloc@PLT
	testq	%rax, %rax
	je	.L49
	addq	$400, 56(%r12)
	movq	%rax, %rbp
.L50:
	subq	$8, %rsp
	.cfi_def_cfa_offset 56
	movl	$100, %edx
	movq	%rbp, %rsi
	xorl	%r8d, %r8d
	pushq	$0
	.cfi_def_cfa_offset 64
	movl	$100, %r9d
	movl	$1, %ecx
	movq	%r12, %rdi
	call	klkfunc_alloc@PLT
	movq	16(%rbx), %rsi
	movq	48(%rax), %r14
	movq	%rax, %r13
	popq	%rax
	.cfi_def_cfa_offset 56
	movq	56(%rbx), %rax
	popq	%rdx
	.cfi_def_cfa_offset 48
	cmpq	%rax, 40(%rbx)
	je	.L62
.L52:
	movq	120(%rbx), %rdi
	leaq	.LC13(%rip), %rsi
	call	klstrpool_new_string@PLT
	testq	%rax, %rax
	je	.L61
	movq	40(%rbx), %rdx
	leaq	16(%rdx), %rcx
	movq	%rcx, 40(%rbx)
	movq	%rax, (%rdx)
	movl	$5, 8(%rdx)
.L53:
	cmpq	48(%rbx), %rdx
	jb	.L55
	movdqu	(%rdx), %xmm1
	leaq	112(%rbx), %rcx
	xorl	%r8d, %r8d
	movq	%r13, %rsi
	movdqa	.LC14(%rip), %xmm0
	movq	%r12, %rdi
	movups	%xmm1, (%r14)
	movups	%xmm0, 0(%rbp)
	movdqa	.LC15(%rip), %xmm0
	movups	%xmm0, 16(%rbp)
	call	klkclosure_create@PLT
	movq	%r13, %rsi
	movq	%r12, %rdi
	movq	%rax, %rbp
	call	klkfunc_initdone@PLT
	movq	40(%rbx), %rdx
	leaq	-16(%rdx), %rax
	cmpq	48(%rbx), %rax
	jb	.L48
	movq	%rbp, -16(%rdx)
	movl	$10, -8(%rdx)
.L48:
	popq	%rbx
	.cfi_remember_state
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	ret
	.p2align 4,,10
	.p2align 3
.L62:
	.cfi_restore_state
	leaq	40(%rbx), %rdi
	call	klstack_expand@PLT
	testb	%al, %al
	jne	.L52
.L61:
	movq	40(%rbx), %rax
	leaq	-16(%rax), %rdx
	jmp	.L53
.L49:
	movq	%r12, %rdi
	call	klmm_do_gc@PLT
	movl	$400, %edi
	call	malloc@PLT
	movq	%rax, %rbp
	testq	%rax, %rax
	je	.L50
	addq	$400, 56(%r12)
	jmp	.L50
	.cfi_endproc
	.section	.text.unlikely
	.cfi_startproc
	.type	concat.cold, @function
concat.cold:
.LFSB183:
.L55:
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	.cfi_offset 6, -40
	.cfi_offset 12, -32
	.cfi_offset 13, -24
	.cfi_offset 14, -16
	movdqa	0, %xmm0
	movups	%xmm0, (%r14)
	ud2
	.cfi_endproc
.LFE183:
	.text
	.size	concat, .-concat
	.section	.text.unlikely
	.size	concat.cold, .-concat.cold
.LCOLDE16:
	.text
.LHOTE16:
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC1:
	.long	-2004811712
	.long	285
	.long	-2130705913
	.long	16843026
	.align 16
.LC2:
	.long	541
	.long	-2113928441
	.long	16843282
	.long	33620225
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC3:
	.long	-2147483343
	.long	256
	.section	.rodata.cst16
	.align 16
.LC5:
	.quad	0
	.quad	1024
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC7:
	.long	1232348160
	.section	.rodata.cst16
	.align 16
.LC14:
	.long	70
	.long	25
	.long	-2147221177
	.long	768
	.align 16
.LC15:
	.long	132096
	.long	33751056
	.long	2147221832
	.long	22
	.ident	"GCC: (GNU) 13.2.1 20230801"
	.section	.note.GNU-stack,"",@progbits
