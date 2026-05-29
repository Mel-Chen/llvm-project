; The stride is 1<<30 (1 GiB)
; Trip count %n is 8 scalar iters
; Vector trip count %n.vec is 4 iters with VF = 2.

target triple = "x86_64-unknown-linux-gnu"

declare i32 @printf(ptr, ...)

; scalar_loop: out_addr[i] = base + i * stride (golden)
define void @scalar_loop(ptr %base, ptr %out_addr, i32 %n) {
entry:
  %nz = icmp eq i32 %n, 0
  br i1 %nz, label %exit, label %loop
loop:
  %iv = phi i32 [ 0, %entry ], [ %iv.next, %loop ]
  %iv64 = zext i32 %iv to i64
  %off = mul i64 %iv64, 1073741824
  %addr = getelementptr i8, ptr %base, i64 %off
  %addri = ptrtoint ptr %addr to i64
  %aslot = getelementptr i64, ptr %out_addr, i32 %iv
  store i64 %addri, ptr %aslot
  %iv.next = add i32 %iv, 1
  %cmp = icmp ult i32 %iv.next, %n
  br i1 %cmp, label %loop, label %exit
exit:
  ret void
}

define void @gather_loop(ptr %base, ptr %out_addr, i32 %n) {
entry:
  %nz = icmp eq i32 %n, 0
  %n.mod.vf = urem i32 %n, 2
  %n.vec = sub i32 %n, %n.mod.vf
  br i1 %nz, label %exit, label %vector.body
vector.body:
  %index = phi i32 [ 0, %entry ], [ %index.next, %vector.body ]
  %vec.ind = phi <2 x i32> [ <i32 0, i32 1>, %entry ], [ %vec.ind.next, %vector.body ]
  %ext = zext <2 x i32> %vec.ind to <2 x i64>
  %offs = shl <2 x i64> %ext, splat (i64 30)
  %ptrs = getelementptr i8, ptr %base, <2 x i64> %offs
  %ip = ptrtoint <2 x ptr> %ptrs to <2 x i64>
  %slot  = getelementptr i64, ptr %out_addr, i32 %index
  store <2 x i64> %ip, ptr %slot, align 8
  %index.next = add nuw i32 %index, 2
  %vec.ind.next = add <2 x i32> %vec.ind, splat (i32 2)
  %done = icmp eq i32 %index.next, %n.vec
  br i1 %done, label %exit, label %vector.body
exit:
  ret void
}

; zext_caniv_loop (my approach): BasePtr = base + zext(canIV) * stride
; Records BasePtr at out_addr[index], and out_addr[index+1] is left as 0 so the
; slot layout still matches a <2 x i64> store.
define void @zext_caniv_loop(ptr %base, ptr %out_addr, i32 %n) {
entry:
  %nz       = icmp eq i32 %n, 0
  %n.mod.vf = urem i32 %n, 2
  %n.vec    = sub i32 %n, %n.mod.vf
  br i1 %nz, label %exit, label %vector.body
vector.body:
  %index = phi i32 [ 0, %entry ], [ %index.next, %vector.body ]
  %iv64  = zext i32 %index to i64
  %off64 = shl i64 %iv64, 30
  %bptr  = getelementptr i8, ptr %base, i64 %off64
  %bint  = ptrtoint ptr %bptr to i64
  %v0    = insertelement <2 x i64> poison, i64 %bint, i32 0
  %v     = insertelement <2 x i64> %v0,    i64 0,     i32 1
  %slot  = getelementptr i64, ptr %out_addr, i32 %index
  store <2 x i64> %v, ptr %slot, align 8
  %index.next = add nuw i32 %index, 2
  %done = icmp eq i32 %index.next, %n.vec
  br i1 %done, label %exit, label %vector.body
exit:
  ret void
}

; trunc_stride_loop (luke's comment): BasePtr = base + sext_i64(canIV * stride in i32)
; Same recording convention as zext_caniv_loop
define void @trunc_stride_loop(ptr %base, ptr %out_addr, i32 %n) {
entry:
  %nz       = icmp eq i32 %n, 0
  %n.mod.vf = urem i32 %n, 2
  %n.vec    = sub i32 %n, %n.mod.vf
  br i1 %nz, label %exit, label %vector.body
vector.body:
  %index = phi i32 [ 0, %entry ], [ %index.next, %vector.body ]
  %off32 = shl i32 %index, 30
  %bptr  = getelementptr i8, ptr %base, i32 %off32
  %bint  = ptrtoint ptr %bptr to i64
  %v0    = insertelement <2 x i64> poison, i64 %bint, i32 0
  %v     = insertelement <2 x i64> %v0,    i64 0,     i32 1
  %slot  = getelementptr i64, ptr %out_addr, i32 %index
  store <2 x i64> %v, ptr %slot, align 8
  %index.next = add nuw i32 %index, 2
  %done = icmp eq i32 %index.next, %n.vec
  br i1 %done, label %exit, label %vector.body
exit:
  ret void
}

; Generated with claude assistance for printing IR.
;-------- printing helpers / main ----------
@.title  = private constant [12 x i8] c"=== %s ===\0A\00"
@.row    = private constant [27 x i8] c"  out_addr[%d] = 0x%016lx\0A\00"
@.nl     = private constant [2  x i8] c"\0A\00"
@.lbl_s  = private constant [21 x i8] c"scalar_loop (golden)\00"
@.lbl_g  = private constant [26 x i8] c"gather_loop (pre-strided)\00"
@.lbl_m  = private constant [21 x i8] c"zext_caniv_loop (my)\00"
@.lbl_l  = private constant [25 x i8] c"trunc_stride_loop (luke)\00"

define void @print_block(ptr %label, ptr %arr) {
entry:
  call i32 (ptr, ...) @printf(ptr @.title, ptr %label)
  br label %lp
lp:
  %i = phi i32 [ 0, %entry ], [ %i.next, %lp ]
  %slot = getelementptr i64, ptr %arr, i32 %i
  %v = load i64, ptr %slot
  call i32 (ptr, ...) @printf(ptr @.row, i32 %i, i64 %v)
  %i.next = add i32 %i, 1
  %done = icmp uge i32 %i.next, 8
  br i1 %done, label %end, label %lp
end:
  call i32 (ptr, ...) @printf(ptr @.nl)
  ret void
}

define i32 @main() {
entry:
  ; Fake base. Pick 0x100000000 (4 GiB) so addresses are easy to read and
  %base = inttoptr i64 4294967296 to ptr

  ; out_addr arrays (8 i64 each)
  %addr_s = alloca [8 x i64], align 8
  %addr_g = alloca [8 x i64], align 8
  %addr_m = alloca [8 x i64], align 8
  %addr_l = alloca [8 x i64], align 8

  call void @scalar_loop      (ptr %base, ptr %addr_s, i32 8)
  call void @gather_loop      (ptr %base, ptr %addr_g, i32 8)
  call void @zext_caniv_loop  (ptr %base, ptr %addr_m, i32 8)
  call void @trunc_stride_loop(ptr %base, ptr %addr_l, i32 8)

  call void @print_block(ptr @.lbl_s, ptr %addr_s)
  call void @print_block(ptr @.lbl_g, ptr %addr_g)
  call void @print_block(ptr @.lbl_m, ptr %addr_m)
  call void @print_block(ptr @.lbl_l, ptr %addr_l)

  ret i32 0
}
