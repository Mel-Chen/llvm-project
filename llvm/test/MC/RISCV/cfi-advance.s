# RUN: llvm-mc -filetype=obj -triple riscv32 -mattr=-relax %s -o %t.o
# RUN: llvm-readelf -sr %t.o | FileCheck %s --check-prefixes=CHECK,NORELAX
# RUN: llvm-dwarfdump --debug-frame %t.o 2>&1 \
# RUN:     | FileCheck -check-prefix=CHECK-DWARFDUMP %s
# RUN: llvm-mc -filetype=obj -triple riscv32 -mattr=+relax %s -o %t.relax.o
# RUN: llvm-readelf -sr %t.relax.o | FileCheck %s --check-prefixes=CHECK,RELAX

# NORELAX:      Relocation section '.rela.text1' at offset {{.*}} contains 1 entries:
# NORELAX-NEXT:  Offset     Info    Type                Sym. Value  Symbol's Name + Addend
# NORELAX-NEXT: 00000000  00000313 R_RISCV_CALL_PLT       00000004   .L0 + 0
# NORELAX-EMPTY:
# RELAX:        Relocation section '.rela.text1' at offset {{.*}} contains 2 entries:
# RELAX:        R_RISCV_CALL_PLT
# RELAX-NEXT:   R_RISCV_RELAX
# RELAX-EMPTY:
# CHECK-NEXT:   Relocation section '.rela.eh_frame' at offset {{.*}} contains 1 entries:
# CHECK:         Offset     Info    Type                Sym. Value  Symbol's Name + Addend
# CHECK-NEXT:   0000001c  00000139 R_RISCV_32_PCREL       00000000   .L0 + 0
# CHECK-EMPTY:
# CHECK:        Symbol table '.symtab' contains 13 entries:
# CHECK-NEXT:      Num:    Value  Size Type    Bind   Vis       Ndx Name
# CHECK-NEXT:        0: 00000000     0 NOTYPE  LOCAL  DEFAULT   UND
# CHECK-NEXT:        1: 00000000     0 NOTYPE  LOCAL  DEFAULT     2 .L0 {{$}}
# CHECK:             3: 00000004     0 NOTYPE  LOCAL  DEFAULT     2 .L0{{$}}
# CHECK-NOT:    .L0

# CHECK-DWARFDUMP: DW_CFA_advance_loc1: 104
# CHECK-DWARFDUMP-NEXT: DW_CFA_def_cfa_offset: +8
# CHECK-DWARFDUMP-NEXT: DW_CFA_advance_loc2: 259
# CHECK-DWARFDUMP-NEXT: DW_CFA_def_cfa_offset: +8
# CHECK-DWARFDUMP-NEXT: DW_CFA_advance_loc4: 65539
# CHECK-DWARFDUMP-NEXT: DW_CFA_def_cfa_offset: +8
# CHECK-DWARFDUMP-NEXT: DW_CFA_advance_loc: 10
# CHECK-DWARFDUMP-NEXT: DW_CFA_def_cfa_offset: +8
        .text
        .globl  test                            # -- Begin function test
        .p2align        1
        .type   test,@function
test:
        .cfi_startproc
        nop
## This looks similar to fake label names ".L0 ". Even if this is ".L0 ",
## the assembler will not conflate it with fake labels.
.L0:
        .zero 100, 0x90
        .cfi_def_cfa_offset 8
        nop
        .zero 255, 0x90
        .cfi_def_cfa_offset 8
        nop
        .zero 65535, 0x90
        .cfi_def_cfa_offset 8
        nop
        .p2align 3
        .cfi_def_cfa_offset 8
        nop
        .cfi_endproc

.section .text1,"ax"
call .L0
