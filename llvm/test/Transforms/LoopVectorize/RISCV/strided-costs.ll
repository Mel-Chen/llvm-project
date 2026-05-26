; REQUIRES: asserts
; RUN: opt -mtriple=riscv64 -mattr=+v -passes=loop-vectorize -mattr=+unaligned-vector-mem \
; RUN:     -debug-only=loop-vectorize --disable-output < %s 2>&1 | FileCheck %s

; CHECK: Cost of 2 for VF vscale x 1: WIDEN-INTRINSIC{{.*}}call llvm.experimental.vp.strided.load
; CHECK: Cost of 4 for VF vscale x 2: WIDEN-INTRINSIC{{.*}}call llvm.experimental.vp.strided.load
; CHECK: Cost of 8 for VF vscale x 4: WIDEN-INTRINSIC{{.*}}call llvm.experimental.vp.strided.load

define void @strided_access_cost(ptr %in, ptr %out) {
entry:
  br label %loop
loop:
  %iv = phi i64 [ 0, %entry ], [ %iv.next, %loop ]
  %scaled = mul nsw i64 %iv, 2
  %gep_in = getelementptr i32, ptr %in, i64 %scaled
  %load = load i32, ptr %gep_in, align 2
  %add = add i32 %load, 1
  %gep_out = getelementptr i32, ptr %out, i64 %iv
  store i32 %add, ptr %gep_out, align 2
  %iv.next = add nuw nsw i64 %iv, 1
  %exitcond = icmp eq i64 %iv.next, 1024
  br i1 %exitcond, label %exit, label %loop
exit:
  ret void
}
