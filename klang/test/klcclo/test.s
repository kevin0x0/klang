	.file	"test.c"
	.text
	.section	.text.unlikely,"x"
.LCOLDB0:
	.text
.LHOTB0:
	.p2align 4
	.globl	fibonacci
	.def	fibonacci;	.scl	2;	.type	32;	.endef
	.seh_proc	fibonacci
fibonacci:
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rdi
	.seh_pushreg	%rdi
	pushq	%rsi
	.seh_pushreg	%rsi
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$40, %rsp
	.seh_stackalloc	40
	.seh_endprologue
	movq	136(%rcx), %rax
	cmpw	$1, 66(%rax)
	movq	%rcx, %rbx
	jne	.L20
	movq	40(%rcx), %rax
	leaq	-16(%rax), %rdx
	cmpq	48(%rcx), %rdx
	jb	.L24
	movq	-16(%rax), %rdi
	cmpq	$1, %rdi
	jg	.L4
	movl	$1, -8(%rax)
.L19:
	xorl	%eax, %eax
.L1:
	addq	$40, %rsp
	popq	%rbx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	ret
	.p2align 4,,10
	.p2align 3
.L24:
	movq	0, %rax
	ud2
	.p2align 4,,10
	.p2align 3
.L4:
	movq	56(%rcx), %rdx
	movq	16(%rcx), %r8
	leaq	40(%rcx), %rsi
	cmpq	%rdx, %rax
	je	.L25
.L5:
	leaq	16(%rax), %rcx
	leaq	fibonacci(%rip), %r10
	movq	%rcx, 40(%rbx)
	movq	%r10, (%rax)
	movl	$3, 8(%rax)
.L7:
	subq	$1, %rdi
	cmpq	%rcx, %rdx
	je	.L26
.L8:
	leaq	16(%rcx), %rdx
	movq	%rdx, 40(%rbx)
	movq	%rdi, (%rcx)
	movl	$1, 8(%rcx)
.L10:
	subq	$32, %rdx
	cmpq	48(%rbx), %rdx
	jb	.L27
.L11:
	movl	$1, %r9d
	movl	$1, %r8d
	movq	%rbx, %rcx
	call	klexec_call
	testl	%eax, %eax
	jne	.L1
	movq	40(%rbx), %rax
	movq	48(%rbx), %rcx
	leaq	-48(%rax), %rdx
	cmpq	%rcx, %rdx
	jb	.L18
	movq	-48(%rax), %rdi
	movq	16(%rbx), %rdx
	subq	$2, %rdi
	cmpq	56(%rbx), %rax
	je	.L28
.L13:
	leaq	16(%rax), %rdx
	movq	%rdx, 40(%rbx)
	movq	%rdi, (%rax)
	movl	$1, 8(%rax)
.L15:
	subq	$48, %rdx
	xorl	%eax, %eax
	movl	$1, %r9d
	movl	$1, %r8d
	cmpq	%rcx, %rdx
	movq	%rbx, %rcx
	cmovb	%rax, %rdx
	call	klexec_call
	testl	%eax, %eax
	jne	.L1
	movq	40(%rbx), %rax
	movq	48(%rbx), %rcx
	leaq	-32(%rax), %rdx
	cmpq	%rcx, %rdx
	jb	.L18
	leaq	-16(%rax), %rdx
	movq	-32(%rax), %rdi
	cmpq	%rcx, %rdx
	jb	.L18
	movq	%rax, %rdx
	movq	-16(%rax), %rbp
	movq	16(%rbx), %r8
	subq	%rcx, %rdx
	movl	$3, %ecx
	sarq	$4, %rdx
	cmpq	%rcx, %rdx
	cmova	%rcx, %rdx
	leaq	112(%rbx), %rcx
	addq	%rbp, %rdi
	salq	$4, %rdx
	subq	%rdx, %rax
	movq	%rax, %rsi
	movq	%rax, %rdx
	call	klref_close
	movq	%rsi, 40(%rbx)
	leaq	-16(%rsi), %rax
	cmpq	48(%rbx), %rax
	jb	.L19
	movq	%rdi, -16(%rsi)
	xorl	%eax, %eax
	movl	$1, -8(%rsi)
	jmp	.L1
	.p2align 4,,10
	.p2align 3
.L20:
	movl	$5, %eax
	addq	$40, %rsp
	popq	%rbx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	ret
	.p2align 4,,10
	.p2align 3
.L27:
	xorl	%edx, %edx
	jmp	.L11
	.p2align 4,,10
	.p2align 3
.L26:
	movq	%r8, %rdx
	movq	%rsi, %rcx
	call	klstack_expand
	testb	%al, %al
	je	.L29
	movq	40(%rbx), %rcx
	jmp	.L8
	.p2align 4,,10
	.p2align 3
.L25:
	movq	%r8, %rdx
	movq	%rsi, %rcx
	call	klstack_expand
	testb	%al, %al
	je	.L30
	movq	40(%rbx), %rax
	movq	16(%rbx), %r8
	movq	56(%rbx), %rdx
	jmp	.L5
.L28:
	movq	%rsi, %rcx
	call	klstack_expand
	testb	%al, %al
	je	.L31
	movq	40(%rbx), %rax
	movq	48(%rbx), %rcx
	jmp	.L13
.L30:
	movq	16(%rbx), %r8
	movq	40(%rbx), %rcx
	movq	56(%rbx), %rdx
	jmp	.L7
.L29:
	movq	40(%rbx), %rdx
	jmp	.L10
.L31:
	movq	40(%rbx), %rdx
	movq	48(%rbx), %rcx
	jmp	.L15
	.seh_endproc
	.section	.text.unlikely,"x"
	.def	fibonacci.cold;	.scl	3;	.type	32;	.endef
	.seh_proc	fibonacci.cold
	.seh_stackalloc	72
	.seh_savereg	%rbx, 40
	.seh_savereg	%rsi, 48
	.seh_savereg	%rdi, 56
	.seh_savereg	%rbp, 64
	.seh_endprologue
fibonacci.cold:
.L18:
	movq	0, %rax
	ud2
	.text
	.section	.text.unlikely,"x"
	.seh_endproc
.LCOLDE0:
	.text
.LHOTE0:
	.ident	"GCC: (GNU) 12.2.0"
	.def	klexec_call;	.scl	2;	.type	32;	.endef
	.def	klref_close;	.scl	2;	.type	32;	.endef
	.def	klstack_expand;	.scl	2;	.type	32;	.endef
