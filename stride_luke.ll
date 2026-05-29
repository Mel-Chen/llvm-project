define i32 @trunc_stride_loop(ptr %base, ptr %out_addr, i32 %n) {
entry:
  %nz = icmp eq i32 %n, 0
  %n.mod.vf = urem i32 %n, 2
  %n.vec = sub i32 %n, %n.mod.vf
  br i1 %nz, label %exit, label %vector.body

vector.body:
  %index = phi i32 [ 0, %entry ], [ %index.next, %vector.body ]
  %vec.ind = phi <2 x i32> [ <i32 0, i32 1>, %entry ], [ %vec.ind.next, %vector.body ]
  %vec.phi = phi <2 x i32> [ zeroinitializer, %entry ], [ %14, %vector.body ]
  %8 = zext <2 x i32> %vec.ind to <2 x i64>
  %9 = shl <2 x i64> %8, splat (i64 30)
  %10 = getelementptr i8, ptr %base, <2 x i64> %9
  %11 = shl i32 %index, 30
  %12 = getelementptr i8, ptr %base, i32 %11
  %13 = call <2 x i32> @llvm.experimental.vp.strided.load.v2i32.p0.i64(ptr align 4 %12, i64 1073741824, <2 x i1> splat (i1 true), i32 2)
  %14 = add <2 x i32> %vec.phi, %13
  %15 = ptrtoint <2 x ptr> %10 to <2 x i64>
  %16 = getelementptr i64, ptr %out_addr, i32 %index
  store <2 x i64> %15, ptr %16, align 8
  %index.next = add nuw i32 %index, 2
  %vec.ind.next = add <2 x i32> %vec.ind, splat (i32 2)
  %17 = icmp eq i32 %index.next, %n.vec
  br i1 %17, label %middle.block, label %vector.body

middle.block:
  %18 = call i32 @llvm.vector.reduce.add.v2i32(<2 x i32> %14)
  %cmp.n = icmp eq i32 %n, %n.vec
  br i1 %cmp.n, label %exit.loopexit, label %scalar.ph

scalar.ph:
  %bc.resume.val = phi i32 [ %n.vec, %middle.block ]
  %bc.merge.rdx = phi i32 [ %18, %middle.block ]
  br label %loop

loop:
  %iv = phi i32 [ %iv.next, %loop ], [ %bc.resume.val, %scalar.ph ]
  %acc = phi i32 [ %acc.next, %loop ], [ %bc.merge.rdx, %scalar.ph ]
  %iv64 = zext i32 %iv to i64
  %off = mul i64 %iv64, 1073741824
  %addr = getelementptr i8, ptr %base, i64 %off
  %val = load i32, ptr %addr, align 4
  %acc.next = add i32 %acc, %val
  %addri = ptrtoint ptr %addr to i64
  %aslot = getelementptr i64, ptr %out_addr, i32 %iv
  store i64 %addri, ptr %aslot, align 8
  %iv.next = add i32 %iv, 1
  %cmp = icmp ult i32 %iv.next, %n
  br i1 %cmp, label %loop, label %exit.loopexit

exit.loopexit:
  %acc.next.lcssa = phi i32 [ %acc.next, %loop ], [ %18, %middle.block ]
  br label %exit

exit:
  %ret = phi i32 [ 0, %entry ], [ %acc.next.lcssa, %exit.loopexit ]
  ret i32 %ret
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <2 x i32> @llvm.experimental.vp.strided.load.v2i32.p0.i64(ptr captures(none), i64, <2 x i1>, i32) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v2i32(<2 x i32>) #1

attributes #0 = { "target-features"="+v" }
attributes #1 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(argmem: read) }
