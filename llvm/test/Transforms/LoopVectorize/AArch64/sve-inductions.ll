; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --filter-out-after "^scalar.ph:"
; RUN: opt -passes=loop-vectorize,dce,instcombine -force-target-instruction-cost=1 \
; RUN:   -prefer-predicate-over-epilogue=scalar-epilogue < %s -S | FileCheck %s

target triple = "aarch64-linux-gnu"

; Test a case where the vectorised induction variable is used to
; generate a mask:
;   for (long long i = 0; i < n; i++) {
;     if (i & 0x1)
;       a[i] = b[i];
;   }

define void @cond_ind64(ptr noalias nocapture %a, ptr noalias nocapture readonly %b, i64 %n) #0 {
; CHECK-LABEL: @cond_ind64(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = call i64 @llvm.vscale.i64()
; CHECK-NEXT:    [[TMP1:%.*]] = shl nuw i64 [[TMP0]], 2
; CHECK-NEXT:    [[MIN_ITERS_CHECK:%.*]] = icmp ult i64 [[N:%.*]], [[TMP1]]
; CHECK-NEXT:    br i1 [[MIN_ITERS_CHECK]], label [[SCALAR_PH:%.*]], label [[VECTOR_PH:%.*]]
; CHECK:       vector.ph:
; CHECK-NEXT:    [[TMP2:%.*]] = call i64 @llvm.vscale.i64()
; CHECK-NEXT:    [[TMP3:%.*]] = shl nuw i64 [[TMP2]], 2
; CHECK-NEXT:    [[N_MOD_VF:%.*]] = urem i64 [[N]], [[TMP3]]
; CHECK-NEXT:    [[N_VEC:%.*]] = sub i64 [[N]], [[N_MOD_VF]]
; CHECK-NEXT:    [[TMP4:%.*]] = call i64 @llvm.vscale.i64()
; CHECK-NEXT:    [[TMP5:%.*]] = shl nuw i64 [[TMP4]], 2
; CHECK-NEXT:    [[TMP6:%.*]] = call <vscale x 4 x i64> @llvm.stepvector.nxv4i64()
; CHECK-NEXT:    [[DOTSPLATINSERT:%.*]] = insertelement <vscale x 4 x i64> poison, i64 [[TMP5]], i64 0
; CHECK-NEXT:    [[DOTSPLAT:%.*]] = shufflevector <vscale x 4 x i64> [[DOTSPLATINSERT]], <vscale x 4 x i64> poison, <vscale x 4 x i32> zeroinitializer
; CHECK-NEXT:    br label [[VECTOR_BODY:%.*]]
; CHECK:       vector.body:
; CHECK-NEXT:    [[INDEX:%.*]] = phi i64 [ 0, [[VECTOR_PH]] ], [ [[INDEX_NEXT:%.*]], [[VECTOR_BODY]] ]
; CHECK-NEXT:    [[VEC_IND:%.*]] = phi <vscale x 4 x i64> [ [[TMP6]], [[VECTOR_PH]] ], [ [[VEC_IND_NEXT:%.*]], [[VECTOR_BODY]] ]
; CHECK-NEXT:    [[TMP9:%.*]] = trunc <vscale x 4 x i64> [[VEC_IND]] to <vscale x 4 x i1>
; CHECK-NEXT:    [[TMP10:%.*]] = getelementptr i32, ptr [[B:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    [[WIDE_MASKED_LOAD:%.*]] = call <vscale x 4 x i32> @llvm.masked.load.nxv4i32.p0(ptr [[TMP10]], i32 4, <vscale x 4 x i1> [[TMP9]], <vscale x 4 x i32> poison)
; CHECK-NEXT:    [[TMP11:%.*]] = getelementptr i32, ptr [[A:%.*]], i64 [[INDEX]]
; CHECK-NEXT:    call void @llvm.masked.store.nxv4i32.p0(<vscale x 4 x i32> [[WIDE_MASKED_LOAD]], ptr [[TMP11]], i32 4, <vscale x 4 x i1> [[TMP9]])
; CHECK-NEXT:    [[INDEX_NEXT]] = add nuw i64 [[INDEX]], [[TMP5]]
; CHECK-NEXT:    [[VEC_IND_NEXT]] = add <vscale x 4 x i64> [[VEC_IND]], [[DOTSPLAT]]
; CHECK-NEXT:    [[TMP12:%.*]] = icmp eq i64 [[INDEX_NEXT]], [[N_VEC]]
; CHECK-NEXT:    br i1 [[TMP12]], label [[MIDDLE_BLOCK:%.*]], label [[VECTOR_BODY]], !llvm.loop [[LOOP0:![0-9]+]]
; CHECK:       middle.block:
; CHECK-NEXT:    [[CMP_N:%.*]] = icmp eq i64 [[N_MOD_VF]], 0
; CHECK-NEXT:    br i1 [[CMP_N]], label [[EXIT:%.*]], label [[SCALAR_PH]]
; CHECK:       scalar.ph:
;
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.inc
  %i.08 = phi i64 [ %inc, %for.inc ], [ 0, %entry ]
  %and = and i64 %i.08, 1
  %tobool.not = icmp eq i64 %and, 0
  br i1 %tobool.not, label %for.inc, label %if.then

if.then:                                          ; preds = %for.body
  %arrayidx = getelementptr inbounds i32, ptr %b, i64 %i.08
  %0 = load i32, ptr %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, ptr %a, i64 %i.08
  store i32 %0, ptr %arrayidx1, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body, %if.then
  %inc = add nuw nsw i64 %i.08, 1
  %exitcond.not = icmp eq i64 %inc, %n
  br i1 %exitcond.not, label %exit, label %for.body, !llvm.loop !0

exit:                                 ; preds = %for.inc
  ret void
}

attributes #0 = { "target-features"="+sve" }

!0 = distinct !{!0, !1, !2, !3, !4, !5}
!1 = !{!"llvm.loop.mustprogress"}
!2 = !{!"llvm.loop.vectorize.scalable.enable", i1 true}
!3 = !{!"llvm.loop.vectorize.enable", i1 true}
!4 = !{!"llvm.loop.vectorize.width", i32 4}
!5 = !{!"llvm.loop.interleave.count", i32 1}
