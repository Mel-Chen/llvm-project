; RUN: llvm-split -o %t %s -j 3 -mtriple amdgcn-amd-amdhsa
; RUN: llvm-dis -o - %t0 | FileCheck --check-prefix=CHECK0 --implicit-check-not=define %s
; RUN: llvm-dis -o - %t1 | FileCheck --check-prefix=CHECK1 --implicit-check-not=define %s
; RUN: llvm-dis -o - %t2 | FileCheck --check-prefix=CHECK2 --implicit-check-not=define %s

; CHECK0: define internal void @HelperD
; CHECK0: define amdgpu_kernel void @D

; CHECK1: define internal void @HelperC
; CHECK1: define amdgpu_kernel void @C

; CHECK2: define hidden void @HelperA
; CHECK2: define hidden void @HelperB
; CHECK2: define hidden void @CallCandidate
; CHECK2: define internal void @HelperC
; CHECK2: define internal void @HelperD
; CHECK2: define amdgpu_kernel void @A
; CHECK2: define amdgpu_kernel void @B

@addrthief = global [3 x ptr] [ptr @HelperA, ptr @HelperB, ptr @CallCandidate]

define internal void @HelperA(ptr %call) {
  call void %call()
  ret void
}

define internal void @HelperB(ptr %call) {
  call void @HelperC()
  call void %call()
  call void @HelperD()
  ret void
}

define internal void @CallCandidate() {
  ret void
}

define internal void @HelperC() {
  ret void
}

define internal void @HelperD() {
  ret void
}

define amdgpu_kernel void @A(ptr %call) {
  call void @HelperA(ptr %call)
  ret void
}

define amdgpu_kernel void @B(ptr %call) {
  call void @HelperB(ptr %call)
  ret void
}

define amdgpu_kernel void @C() {
  call void @HelperC()
  ret void
}

define amdgpu_kernel void @D() {
  call void @HelperD()
  ret void
}
