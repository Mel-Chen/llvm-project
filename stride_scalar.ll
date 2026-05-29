define i32 @scalar_loop(ptr %base, ptr %out_addr, i32 %n) {
entry:
  %nz = icmp eq i32 %n, 0
  br i1 %nz, label %exit, label %loop
loop:
  %iv = phi i32 [ 0, %entry ], [ %iv.next, %loop ]
  %acc = phi i32 [ 0, %entry ], [ %acc.next, %loop ]
  %iv64 = zext i32 %iv to i64
  %off = mul i64 %iv64, 1073741824
  %addr = getelementptr i8, ptr %base, i64 %off
  %val = load i32, ptr %addr, align 4
  %acc.next = add i32 %acc, %val
  %addri = ptrtoint ptr %addr to i64
  %aslot = getelementptr i64, ptr %out_addr, i32 %iv
  store i64 %addri, ptr %aslot
  %iv.next = add i32 %iv, 1
  %cmp = icmp ult i32 %iv.next, %n
  br i1 %cmp, label %loop, label %exit
exit:
  %ret = phi i32 [ 0, %entry ], [ %acc.next, %loop ]
  ret i32 %ret
}
