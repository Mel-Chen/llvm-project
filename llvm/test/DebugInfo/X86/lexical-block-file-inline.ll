; RUN: llc -mtriple=x86_64-unknown-linux-gnu -filetype=obj -O0 < %s  | llvm-dwarfdump -v -debug-info - | FileCheck %s

;; This test checks that Inlined DILexicalBlockFile with local decl entry
;; is skipped and only one DW_TAG_lexical_block is generated.
;; This test is special because it contains DILexicalBlockFile that has a
;; DILexicalBlockFile as a parent scope.
;;
;; This test was generated by running following command:
;; clang -cc1 -O0 -debug-info-kind=limited -dwarf-version=4 -emit-llvm test.cpp
;; Where test.cpp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;namespace N {}
;; __attribute__((always_inline)) int bar() {
;;  {
;;    int y;
;;#line 1 "test.h"
;;    using namespace N;
;;    while (y < 0) return 2;
;;    return 0;
;;  }
;;}
;;int foo() {  
;;  return bar();
;;}
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Concrete "bar" function
; CHECK:    DW_TAG_subprogram
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:      DW_AT_abstract_origin {{.*}} {[[Offset_bar:0x[0-9abcdef]+]]}
; CHECK-NOT:  {{DW_TAG|NULL}}
; CHECK:      DW_TAG_lexical_block
; CHECK-NOT:    {{DW_TAG|NULL}}
; CHECK:        DW_AT_abstract_origin {{.*}}[[Offset_lb:0x[0-9a-f]+]]
; CHECK-NOT:    {{DW_TAG|NULL}}
; CHECK:        DW_TAG_variable

;; Abstract "bar" function
; CHECK:    [[Offset_bar]]: DW_TAG_subprogram
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:      DW_AT_name {{.*}} "bar"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:      DW_AT_inline
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:      [[Offset_lb]]: DW_TAG_lexical_block
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:        DW_TAG_variable
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:        DW_TAG_imported_module

; CHECK:    DW_TAG_subprogram
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:      DW_AT_name {{.*}} "foo"
; CHECK-NOT: {{NULL}}

;; Inlined "bar" function
; CHECK:      DW_TAG_inlined_subroutine
; CHECK-NEXT:   DW_AT_abstract_origin {{.*}} {[[Offset_bar]]}
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:        DW_TAG_lexical_block
; CHECK-NOT:      {{DW_TAG|NULL}}
; CHECK:          DW_AT_abstract_origin {{.*}}[[Offset_lb]]
; CHECK-NOT:    {{DW_TAG|NULL}}
; CHECK:        DW_TAG_variable

; Function Attrs: alwaysinline nounwind
define i32 @_Z3barv() #0 !dbg !4 {
entry:
  %retval = alloca i32, align 4
  %y = alloca i32, align 4
  call void @llvm.dbg.declare(metadata ptr %y, metadata !18, metadata !19), !dbg !20
  br label %while.cond, !dbg !21

while.cond:                                       ; preds = %entry
  %0 = load i32, ptr %y, align 4, !dbg !22
  %cmp = icmp slt i32 %0, 0, !dbg !22
  br i1 %cmp, label %while.body, label %while.end, !dbg !22

while.body:                                       ; preds = %while.cond
  store i32 2, ptr %retval, align 4, !dbg !24
  br label %return, !dbg !24

while.end:                                        ; preds = %while.cond
  store i32 0, ptr %retval, align 4, !dbg !26
  br label %return, !dbg !26

return:                                           ; preds = %while.end, %while.body
  %1 = load i32, ptr %retval, align 4, !dbg !27
  ret i32 %1, !dbg !27
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind
define i32 @_Z3foov() #2 !dbg !8 {
entry:
  %retval.i = alloca i32, align 4
  %y.i = alloca i32, align 4
  call void @llvm.dbg.declare(metadata ptr %y.i, metadata !18, metadata !19), !dbg !29
  %0 = load i32, ptr %y.i, align 4, !dbg !31
  %cmp.i = icmp slt i32 %0, 0, !dbg !31
  br i1 %cmp.i, label %while.body.i, label %while.end.i, !dbg !31

while.body.i:                                     ; preds = %entry
  store i32 2, ptr %retval.i, align 4, !dbg !32
  br label %_Z3barv.exit, !dbg !32

while.end.i:                                      ; preds = %entry
  store i32 0, ptr %retval.i, align 4, !dbg !33
  br label %_Z3barv.exit, !dbg !33

_Z3barv.exit:                                     ; preds = %while.end.i, %while.body.i
  %1 = load i32, ptr %retval.i, align 4, !dbg !34
  ret i32 %1, !dbg !35
}

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!15, !16}
!llvm.ident = !{!17}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !1, producer: "clang version 3.9.0 (trunk 264349)", isOptimized: false, runtimeVersion: 0, emissionKind: 1, enums: !2)
!1 = !DIFile(filename: "test.cpp", directory: "/")
!2 = !{}
!4 = distinct !DISubprogram(name: "bar", linkageName: "_Z3barv", scope: !1, file: !1, line: 2, type: !5, isLocal: false, isDefinition: true, scopeLine: 2, flags: DIFlagPrototyped, isOptimized: false, unit: !0, retainedNodes: !10)
!5 = !DISubroutineType(types: !6)
!6 = !{!7}
!7 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!8 = distinct !DISubprogram(name: "foo", linkageName: "_Z3foov", scope: !9, file: !9, line: 6, type: !5, isLocal: false, isDefinition: true, scopeLine: 6, flags: DIFlagPrototyped, isOptimized: false, unit: !0, retainedNodes: !2)
!9 = !DIFile(filename: "test.h", directory: "/")
!10 = !{!11}
!11 = !DIImportedEntity(tag: DW_TAG_imported_module, scope: !12, entity: !14, file: !1, line: 1)
!12 = !DILexicalBlockFile(scope: !13, file: !9, discriminator: 0)
!13 = distinct !DILexicalBlock(scope: !4, file: !1, line: 3)
!14 = !DINamespace(name: "N", scope: null)
!15 = !{i32 2, !"Dwarf Version", i32 4}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = !{!"clang version 3.9.0 (trunk 264349)"}
!18 = !DILocalVariable(name: "y", scope: !13, file: !1, line: 4, type: !7)
!19 = !DIExpression()
!20 = !DILocation(line: 4, scope: !13)
!21 = !DILocation(line: 2, scope: !12)
!22 = !DILocation(line: 2, scope: !23)
!23 = !DILexicalBlockFile(scope: !12, file: !9, discriminator: 1)
!24 = !DILocation(line: 2, scope: !25)
!25 = !DILexicalBlockFile(scope: !12, file: !9, discriminator: 2)
!26 = !DILocation(line: 3, scope: !12)
!27 = !DILocation(line: 5, scope: !28)
!28 = !DILexicalBlockFile(scope: !4, file: !9, discriminator: 0)
!29 = !DILocation(line: 4, scope: !13, inlinedAt: !30)
!30 = distinct !DILocation(line: 7, scope: !8)
!31 = !DILocation(line: 2, scope: !23, inlinedAt: !30)
!32 = !DILocation(line: 2, scope: !25, inlinedAt: !30)
!33 = !DILocation(line: 3, scope: !12, inlinedAt: !30)
!34 = !DILocation(line: 5, scope: !28, inlinedAt: !30)
!35 = !DILocation(line: 7, scope: !8)
