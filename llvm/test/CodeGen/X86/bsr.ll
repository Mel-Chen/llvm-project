; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown | FileCheck %s --check-prefixes=X86
; RUN: llc < %s -mtriple=x86_64-unknown-unknown | FileCheck %s --check-prefixes=X64

define i8 @cmov_bsr8(i8 %x, i8 %y) nounwind {
; X86-LABEL: cmov_bsr8:
; X86:       # %bb.0:
; X86-NEXT:    movzbl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    testb %cl, %cl
; X86-NEXT:    je .LBB0_1
; X86-NEXT:  # %bb.2: # %cond.false
; X86-NEXT:    movzbl %cl, %eax
; X86-NEXT:    bsrl %eax, %eax
; X86-NEXT:    xorl $7, %eax
; X86-NEXT:    testb %cl, %cl
; X86-NEXT:    je .LBB0_4
; X86-NEXT:  .LBB0_5: # %cond.end
; X86-NEXT:    xorb $7, %al
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    retl
; X86-NEXT:  .LBB0_1:
; X86-NEXT:    movb $8, %al
; X86-NEXT:    testb %cl, %cl
; X86-NEXT:    jne .LBB0_5
; X86-NEXT:  .LBB0_4:
; X86-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr8:
; X64:       # %bb.0:
; X64-NEXT:    movzbl %dil, %ecx
; X64-NEXT:    movl $15, %eax
; X64-NEXT:    bsrl %ecx, %eax
; X64-NEXT:    testb %cl, %cl
; X64-NEXT:    cmovel %esi, %eax
; X64-NEXT:    # kill: def $al killed $al killed $eax
; X64-NEXT:    retq
  %1 = tail call i8 @llvm.ctlz.i8(i8 %x, i1 false)
  %2 = xor i8 %1, 7
  %3 = icmp eq i8 %x, 0
  %4 = select i1 %3, i8 %y, i8 %2
  ret i8 %4
}

define i8 @cmov_bsr8_undef(i8 %x, i8 %y) nounwind {
; X86-LABEL: cmov_bsr8_undef:
; X86:       # %bb.0:
; X86-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    testl %eax, %eax
; X86-NEXT:    jne .LBB1_1
; X86-NEXT:  # %bb.2:
; X86-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    retl
; X86-NEXT:  .LBB1_1:
; X86-NEXT:    bsrl %eax, %eax
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr8_undef:
; X64:       # %bb.0:
; X64-NEXT:    movzbl %dil, %ecx
; X64-NEXT:    bsrl %ecx, %eax
; X64-NEXT:    testb %cl, %cl
; X64-NEXT:    cmovel %esi, %eax
; X64-NEXT:    # kill: def $al killed $al killed $eax
; X64-NEXT:    retq
  %1 = tail call i8 @llvm.ctlz.i8(i8 %x, i1 true)
  %2 = xor i8 %1, 7
  %3 = icmp ne i8 %x, 0
  %4 = select i1 %3, i8 %2, i8 %y
  ret i8 %4
}

define i16 @cmov_bsr16(i16 %x, i16 %y) nounwind {
; X86-LABEL: cmov_bsr16:
; X86:       # %bb.0:
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    testw %ax, %ax
; X86-NEXT:    je .LBB2_1
; X86-NEXT:  # %bb.2: # %cond.false
; X86-NEXT:    bsrw %ax, %cx
; X86-NEXT:    xorl $15, %ecx
; X86-NEXT:    testw %ax, %ax
; X86-NEXT:    jne .LBB2_4
; X86-NEXT:  .LBB2_5: # %cond.end
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    # kill: def $ax killed $ax killed $eax
; X86-NEXT:    retl
; X86-NEXT:  .LBB2_1:
; X86-NEXT:    movw $16, %cx
; X86-NEXT:    testw %ax, %ax
; X86-NEXT:    je .LBB2_5
; X86-NEXT:  .LBB2_4:
; X86-NEXT:    movzwl %cx, %eax
; X86-NEXT:    xorl $15, %eax
; X86-NEXT:    # kill: def $ax killed $ax killed $eax
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr16:
; X64:       # %bb.0:
; X64-NEXT:    movw $31, %ax
; X64-NEXT:    bsrw %di, %ax
; X64-NEXT:    cmovel %esi, %eax
; X64-NEXT:    # kill: def $ax killed $ax killed $eax
; X64-NEXT:    retq
  %1 = tail call i16 @llvm.ctlz.i16(i16 %x, i1 false)
  %2 = xor i16 %1, 15
  %3 = icmp ne i16 %x, 0
  %4 = select i1 %3, i16 %2, i16 %y
  ret i16 %4
}

define i16 @cmov_bsr16_undef(i16 %x, i16 %y) nounwind {
; X86-LABEL: cmov_bsr16_undef:
; X86:       # %bb.0:
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    testw %ax, %ax
; X86-NEXT:    je .LBB3_1
; X86-NEXT:  # %bb.2:
; X86-NEXT:    bsrw %ax, %ax
; X86-NEXT:    retl
; X86-NEXT:  .LBB3_1:
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr16_undef:
; X64:       # %bb.0:
; X64-NEXT:    bsrw %di, %ax
; X64-NEXT:    cmovel %esi, %eax
; X64-NEXT:    # kill: def $ax killed $ax killed $eax
; X64-NEXT:    retq
  %1 = tail call i16 @llvm.ctlz.i16(i16 %x, i1 true)
  %2 = xor i16 %1, 15
  %3 = icmp eq i16 %x, 0
  %4 = select i1 %3, i16 %y, i16 %2
  ret i16 %4
}

define i32 @cmov_bsr32(i32 %x, i32 %y) nounwind {
; X86-LABEL: cmov_bsr32:
; X86:       # %bb.0:
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    testl %ecx, %ecx
; X86-NEXT:    je .LBB4_1
; X86-NEXT:  # %bb.2: # %cond.false
; X86-NEXT:    bsrl %ecx, %eax
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    testl %ecx, %ecx
; X86-NEXT:    je .LBB4_4
; X86-NEXT:  .LBB4_5: # %cond.end
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    retl
; X86-NEXT:  .LBB4_1:
; X86-NEXT:    movl $32, %eax
; X86-NEXT:    testl %ecx, %ecx
; X86-NEXT:    jne .LBB4_5
; X86-NEXT:  .LBB4_4:
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr32:
; X64:       # %bb.0:
; X64-NEXT:    movl %esi, %eax
; X64-NEXT:    bsrl %edi, %eax
; X64-NEXT:    retq
  %1 = tail call i32 @llvm.ctlz.i32(i32 %x, i1 false)
  %2 = xor i32 %1, 31
  %3 = icmp eq i32 %x, 0
  %4 = select i1 %3, i32 %y, i32 %2
  ret i32 %4
}

define i32 @cmov_bsr32_undef(i32 %x, i32 %y) nounwind {
; X86-LABEL: cmov_bsr32_undef:
; X86:       # %bb.0:
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    testl %eax, %eax
; X86-NEXT:    jne .LBB5_1
; X86-NEXT:  # %bb.2:
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    retl
; X86-NEXT:  .LBB5_1:
; X86-NEXT:    bsrl %eax, %eax
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr32_undef:
; X64:       # %bb.0:
; X64-NEXT:    movl %esi, %eax
; X64-NEXT:    bsrl %edi, %eax
; X64-NEXT:    retq
  %1 = tail call i32 @llvm.ctlz.i32(i32 %x, i1 true)
  %2 = xor i32 %1, 31
  %3 = icmp ne i32 %x, 0
  %4 = select i1 %3, i32 %2, i32 %y
  ret i32 %4
}

define i64 @cmov_bsr64(i64 %x, i64 %y) nounwind {
; X86-LABEL: cmov_bsr64:
; X86:       # %bb.0:
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movl {{[0-9]+}}(%esp), %esi
; X86-NEXT:    xorl %edx, %edx
; X86-NEXT:    movl %esi, %eax
; X86-NEXT:    orl %ecx, %eax
; X86-NEXT:    je .LBB6_1
; X86-NEXT:  # %bb.2: # %cond.false
; X86-NEXT:    testl %ecx, %ecx
; X86-NEXT:    jne .LBB6_3
; X86-NEXT:  # %bb.4: # %cond.false
; X86-NEXT:    bsrl %esi, %eax
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    orl $32, %eax
; X86-NEXT:    orl %ecx, %esi
; X86-NEXT:    je .LBB6_7
; X86-NEXT:    jmp .LBB6_6
; X86-NEXT:  .LBB6_1:
; X86-NEXT:    movl $64, %eax
; X86-NEXT:    orl %ecx, %esi
; X86-NEXT:    jne .LBB6_6
; X86-NEXT:  .LBB6_7: # %cond.end
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
; X86-NEXT:  .LBB6_3:
; X86-NEXT:    bsrl %ecx, %eax
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    orl %ecx, %esi
; X86-NEXT:    je .LBB6_7
; X86-NEXT:  .LBB6_6:
; X86-NEXT:    xorl $63, %eax
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr64:
; X64:       # %bb.0:
; X64-NEXT:    movq %rsi, %rax
; X64-NEXT:    bsrq %rdi, %rax
; X64-NEXT:    retq
  %1 = tail call i64 @llvm.ctlz.i64(i64 %x, i1 false)
  %2 = xor i64 %1, 63
  %3 = icmp ne i64 %x, 0
  %4 = select i1 %3, i64 %2, i64 %y
  ret i64 %4
}

define i64 @cmov_bsr64_undef(i64 %x, i64 %y) nounwind {
; X86-LABEL: cmov_bsr64_undef:
; X86:       # %bb.0:
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    testl %edx, %edx
; X86-NEXT:    jne .LBB7_1
; X86-NEXT:  # %bb.2:
; X86-NEXT:    bsrl %ecx, %eax
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    orl $32, %eax
; X86-NEXT:    orl %edx, %ecx
; X86-NEXT:    jne .LBB7_5
; X86-NEXT:  .LBB7_4:
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    retl
; X86-NEXT:  .LBB7_1:
; X86-NEXT:    bsrl %edx, %eax
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    orl %edx, %ecx
; X86-NEXT:    je .LBB7_4
; X86-NEXT:  .LBB7_5:
; X86-NEXT:    xorl $63, %eax
; X86-NEXT:    xorl %edx, %edx
; X86-NEXT:    retl
;
; X64-LABEL: cmov_bsr64_undef:
; X64:       # %bb.0:
; X64-NEXT:    movq %rsi, %rax
; X64-NEXT:    bsrq %rdi, %rax
; X64-NEXT:    retq
  %1 = tail call i64 @llvm.ctlz.i64(i64 %x, i1 true)
  %2 = xor i64 %1, 63
  %3 = icmp eq i64 %x, 0
  %4 = select i1 %3, i64 %y, i64 %2
  ret i64 %4
}

define i128 @cmov_bsr128(i128 %x, i128 %y) nounwind {
; X86-LABEL: cmov_bsr128:
; X86:       # %bb.0:
; X86-NEXT:    pushl %ebp
; X86-NEXT:    movl %esp, %ebp
; X86-NEXT:    pushl %ebx
; X86-NEXT:    pushl %edi
; X86-NEXT:    pushl %esi
; X86-NEXT:    andl $-16, %esp
; X86-NEXT:    subl $16, %esp
; X86-NEXT:    movl 32(%ebp), %ebx
; X86-NEXT:    movl 24(%ebp), %ecx
; X86-NEXT:    movl 36(%ebp), %esi
; X86-NEXT:    movl 28(%ebp), %edi
; X86-NEXT:    movl %edi, %eax
; X86-NEXT:    orl %esi, %eax
; X86-NEXT:    movl %ecx, %edx
; X86-NEXT:    orl %ebx, %edx
; X86-NEXT:    orl %eax, %edx
; X86-NEXT:    je .LBB8_1
; X86-NEXT:  # %bb.2: # %cond.false
; X86-NEXT:    testl %esi, %esi
; X86-NEXT:    jne .LBB8_3
; X86-NEXT:  # %bb.4: # %cond.false
; X86-NEXT:    bsrl %ebx, %esi
; X86-NEXT:    xorl $31, %esi
; X86-NEXT:    orl $32, %esi
; X86-NEXT:    testl %edi, %edi
; X86-NEXT:    je .LBB8_7
; X86-NEXT:  .LBB8_6:
; X86-NEXT:    bsrl %edi, %eax
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    jmp .LBB8_8
; X86-NEXT:  .LBB8_1:
; X86-NEXT:    xorl %eax, %eax
; X86-NEXT:    movl $128, %esi
; X86-NEXT:    jmp .LBB8_11
; X86-NEXT:  .LBB8_3:
; X86-NEXT:    bsrl %esi, %esi
; X86-NEXT:    xorl $31, %esi
; X86-NEXT:    testl %edi, %edi
; X86-NEXT:    jne .LBB8_6
; X86-NEXT:  .LBB8_7: # %cond.false
; X86-NEXT:    bsrl %ecx, %eax
; X86-NEXT:    xorl $31, %eax
; X86-NEXT:    orl $32, %eax
; X86-NEXT:  .LBB8_8: # %cond.false
; X86-NEXT:    movl %ebx, %edx
; X86-NEXT:    orl 36(%ebp), %edx
; X86-NEXT:    jne .LBB8_10
; X86-NEXT:  # %bb.9: # %cond.false
; X86-NEXT:    orl $64, %eax
; X86-NEXT:    movl %eax, %esi
; X86-NEXT:  .LBB8_10: # %cond.false
; X86-NEXT:    xorl %eax, %eax
; X86-NEXT:  .LBB8_11: # %cond.end
; X86-NEXT:    xorl %ebx, %ebx
; X86-NEXT:    xorl %edx, %edx
; X86-NEXT:    orl 32(%ebp), %ecx
; X86-NEXT:    orl 36(%ebp), %edi
; X86-NEXT:    orl %ecx, %edi
; X86-NEXT:    je .LBB8_12
; X86-NEXT:  # %bb.13: # %cond.end
; X86-NEXT:    xorl $127, %esi
; X86-NEXT:    movl %eax, %ecx
; X86-NEXT:    jmp .LBB8_14
; X86-NEXT:  .LBB8_12:
; X86-NEXT:    movl 52(%ebp), %edx
; X86-NEXT:    movl 48(%ebp), %ebx
; X86-NEXT:    movl 44(%ebp), %ecx
; X86-NEXT:    movl 40(%ebp), %esi
; X86-NEXT:  .LBB8_14: # %cond.end
; X86-NEXT:    movl 8(%ebp), %eax
; X86-NEXT:    movl %edx, 12(%eax)
; X86-NEXT:    movl %ebx, 8(%eax)
; X86-NEXT:    movl %ecx, 4(%eax)
; X86-NEXT:    movl %esi, (%eax)
; X86-NEXT:    leal -12(%ebp), %esp
; X86-NEXT:    popl %esi
; X86-NEXT:    popl %edi
; X86-NEXT:    popl %ebx
; X86-NEXT:    popl %ebp
; X86-NEXT:    retl $4
;
; X64-LABEL: cmov_bsr128:
; X64:       # %bb.0:
; X64-NEXT:    bsrq %rsi, %r8
; X64-NEXT:    xorq $63, %r8
; X64-NEXT:    movl $127, %eax
; X64-NEXT:    bsrq %rdi, %rax
; X64-NEXT:    xorq $63, %rax
; X64-NEXT:    addq $64, %rax
; X64-NEXT:    testq %rsi, %rsi
; X64-NEXT:    cmovneq %r8, %rax
; X64-NEXT:    xorq $127, %rax
; X64-NEXT:    xorl %r8d, %r8d
; X64-NEXT:    orq %rsi, %rdi
; X64-NEXT:    cmoveq %rdx, %rax
; X64-NEXT:    cmoveq %rcx, %r8
; X64-NEXT:    movq %r8, %rdx
; X64-NEXT:    retq
  %1 = tail call i128 @llvm.ctlz.i128(i128 %x, i1 false)
  %2 = xor i128 %1, 127
  %3 = icmp eq i128 %x, 0
  %4 = select i1 %3, i128 %y, i128 %2
  ret i128 %4
}

define i128 @cmov_bsr128_undef(i128 %x, i128 %y) nounwind {
; X86-LABEL: cmov_bsr128_undef:
; X86:       # %bb.0:
; X86-NEXT:    pushl %ebp
; X86-NEXT:    movl %esp, %ebp
; X86-NEXT:    pushl %ebx
; X86-NEXT:    pushl %edi
; X86-NEXT:    pushl %esi
; X86-NEXT:    andl $-16, %esp
; X86-NEXT:    subl $16, %esp
; X86-NEXT:    movl 28(%ebp), %edx
; X86-NEXT:    movl 32(%ebp), %edi
; X86-NEXT:    movl 36(%ebp), %eax
; X86-NEXT:    testl %eax, %eax
; X86-NEXT:    jne .LBB9_1
; X86-NEXT:  # %bb.2:
; X86-NEXT:    bsrl %edi, %esi
; X86-NEXT:    xorl $31, %esi
; X86-NEXT:    orl $32, %esi
; X86-NEXT:    jmp .LBB9_3
; X86-NEXT:  .LBB9_1:
; X86-NEXT:    bsrl %eax, %esi
; X86-NEXT:    xorl $31, %esi
; X86-NEXT:  .LBB9_3:
; X86-NEXT:    movl 24(%ebp), %ebx
; X86-NEXT:    testl %edx, %edx
; X86-NEXT:    jne .LBB9_4
; X86-NEXT:  # %bb.5:
; X86-NEXT:    bsrl %ebx, %ecx
; X86-NEXT:    xorl $31, %ecx
; X86-NEXT:    orl $32, %ecx
; X86-NEXT:    orl %eax, %edi
; X86-NEXT:    je .LBB9_7
; X86-NEXT:    jmp .LBB9_8
; X86-NEXT:  .LBB9_4:
; X86-NEXT:    bsrl %edx, %ecx
; X86-NEXT:    xorl $31, %ecx
; X86-NEXT:    orl %eax, %edi
; X86-NEXT:    jne .LBB9_8
; X86-NEXT:  .LBB9_7:
; X86-NEXT:    orl $64, %ecx
; X86-NEXT:    movl %ecx, %esi
; X86-NEXT:  .LBB9_8:
; X86-NEXT:    orl %eax, %edx
; X86-NEXT:    orl 32(%ebp), %ebx
; X86-NEXT:    orl %edx, %ebx
; X86-NEXT:    jne .LBB9_9
; X86-NEXT:  # %bb.10:
; X86-NEXT:    movl 48(%ebp), %edx
; X86-NEXT:    movl 52(%ebp), %edi
; X86-NEXT:    movl 40(%ebp), %esi
; X86-NEXT:    movl 44(%ebp), %ecx
; X86-NEXT:    jmp .LBB9_11
; X86-NEXT:  .LBB9_9:
; X86-NEXT:    xorl $127, %esi
; X86-NEXT:    xorl %ecx, %ecx
; X86-NEXT:    xorl %edx, %edx
; X86-NEXT:    xorl %edi, %edi
; X86-NEXT:  .LBB9_11:
; X86-NEXT:    movl 8(%ebp), %eax
; X86-NEXT:    movl %edi, 12(%eax)
; X86-NEXT:    movl %edx, 8(%eax)
; X86-NEXT:    movl %ecx, 4(%eax)
; X86-NEXT:    movl %esi, (%eax)
; X86-NEXT:    leal -12(%ebp), %esp
; X86-NEXT:    popl %esi
; X86-NEXT:    popl %edi
; X86-NEXT:    popl %ebx
; X86-NEXT:    popl %ebp
; X86-NEXT:    retl $4
;
; X64-LABEL: cmov_bsr128_undef:
; X64:       # %bb.0:
; X64-NEXT:    bsrq %rsi, %r8
; X64-NEXT:    xorq $63, %r8
; X64-NEXT:    bsrq %rdi, %rax
; X64-NEXT:    xorq $63, %rax
; X64-NEXT:    orq $64, %rax
; X64-NEXT:    testq %rsi, %rsi
; X64-NEXT:    cmovneq %r8, %rax
; X64-NEXT:    xorq $127, %rax
; X64-NEXT:    xorl %r8d, %r8d
; X64-NEXT:    orq %rsi, %rdi
; X64-NEXT:    cmoveq %rdx, %rax
; X64-NEXT:    cmoveq %rcx, %r8
; X64-NEXT:    movq %r8, %rdx
; X64-NEXT:    retq
  %1 = tail call i128 @llvm.ctlz.i128(i128 %x, i1 true)
  %2 = xor i128 %1, 127
  %3 = icmp ne i128 %x, 0
  %4 = select i1 %3, i128 %2, i128 %y
  ret i128 %4
}

declare i8 @llvm.ctlz.i8(i8, i1)
declare i16 @llvm.ctlz.i16(i16, i1)
declare i32 @llvm.ctlz.i32(i32, i1)
declare i64 @llvm.ctlz.i64(i64, i1)
declare i128 @llvm.ctlz.i128(i128, i1)
