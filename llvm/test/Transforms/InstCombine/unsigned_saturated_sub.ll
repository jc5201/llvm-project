; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -instcombine -S < %s | FileCheck %s

; Canonicalization of unsigned saturated subtraction idioms to
; usub.sat() intrinsics is tested here.

declare void @use(i64)
declare void @usei32(i32)
declare void @usei1(i1)

; (a > b) ? a - b : 0 -> usub.sat(a, b)

define i64 @max_sub_ugt(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_ugt(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    ret i64 [[TMP1]]
;
  %cmp = icmp ugt i64 %a, %b
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 %sub ,i64 0
  ret i64 %sel
}

; (a >= b) ? a - b : 0 -> usub.sat(a, b)

define i64 @max_sub_uge(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_uge(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    ret i64 [[TMP1]]
;
  %cmp = icmp uge i64 %a, %b
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 %sub ,i64 0
  ret i64 %sel
}

define i64 @max_sub_uge_extrause1(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_uge_extrause1(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i64 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[SUB:%.*]] = sub i64 [[A]], [[B]]
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i64 0, i64 [[SUB]]
; CHECK-NEXT:    call void @use(i64 [[SUB]])
; CHECK-NEXT:    ret i64 [[SEL]]
;
  %cmp = icmp uge i64 %a, %b
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 %sub ,i64 0
  call void @use(i64 %sub)
  ret i64 %sel
}

define i64 @max_sub_uge_extrause2(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_uge_extrause2(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i64 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A]], i64 [[B]])
; CHECK-NEXT:    call void @usei1(i1 [[CMP]])
; CHECK-NEXT:    ret i64 [[TMP1]]
;
  %cmp = icmp uge i64 %a, %b
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 %sub ,i64 0
  call void @usei1(i1 %cmp)
  ret i64 %sel
}

define i64 @max_sub_uge_extrause3(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_uge_extrause3(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i64 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[SUB:%.*]] = sub i64 [[A]], [[B]]
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i64 [[SUB]], i64 0
; CHECK-NEXT:    call void @use(i64 [[SUB]])
; CHECK-NEXT:    call void @usei1(i1 [[CMP]])
; CHECK-NEXT:    ret i64 [[SEL]]
;
  %cmp = icmp uge i64 %a, %b
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 %sub ,i64 0
  call void @use(i64 %sub)
  call void @usei1(i1 %cmp)
  ret i64 %sel
}

; Again, with vectors:
; (a > b) ? a - b : 0 -> usub.sat(a, b)

define <4 x i32> @max_sub_ugt_vec(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: @max_sub_ugt_vec(
; CHECK-NEXT:    [[TMP1:%.*]] = call <4 x i32> @llvm.usub.sat.v4i32(<4 x i32> [[A:%.*]], <4 x i32> [[B:%.*]])
; CHECK-NEXT:    ret <4 x i32> [[TMP1]]
;
  %cmp = icmp ugt <4 x i32> %a, %b
  %sub = sub <4 x i32> %a, %b
  %sel = select <4 x i1> %cmp, <4 x i32> %sub, <4 x i32> zeroinitializer
  ret <4 x i32> %sel
}

; Use extra ops to thwart icmp swapping canonicalization.
; (b < a) ? a - b : 0 -> usub.sat(a, b)

define i64 @max_sub_ult(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_ult(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    [[EXTRASUB:%.*]] = sub i64 [[B]], [[A]]
; CHECK-NEXT:    call void @use(i64 [[EXTRASUB]])
; CHECK-NEXT:    ret i64 [[TMP1]]
;
  %cmp = icmp ult i64 %b, %a
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 %sub ,i64 0
  %extrasub = sub i64 %b, %a
  call void @use(i64 %extrasub)
  ret i64 %sel
}

; (b > a) ? 0 : a - b -> usub.sat(a, b)

define i64 @max_sub_ugt_sel_swapped(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_ugt_sel_swapped(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    [[EXTRASUB:%.*]] = sub i64 [[B]], [[A]]
; CHECK-NEXT:    call void @use(i64 [[EXTRASUB]])
; CHECK-NEXT:    ret i64 [[TMP1]]
;
  %cmp = icmp ugt i64 %b, %a
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 0 ,i64 %sub
  %extrasub = sub i64 %b, %a
  call void @use(i64 %extrasub)
  ret i64 %sel
}

; (a < b) ? 0 : a - b -> usub.sat(a, b)

define i64 @max_sub_ult_sel_swapped(i64 %a, i64 %b) {
; CHECK-LABEL: @max_sub_ult_sel_swapped(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    ret i64 [[TMP1]]
;
  %cmp = icmp ult i64 %a, %b
  %sub = sub i64 %a, %b
  %sel = select i1 %cmp, i64 0 ,i64 %sub
  ret i64 %sel
}

; ((a > b) ? b - a : 0) -> -usub.sat(a, b)

define i64 @neg_max_sub_ugt(i64 %a, i64 %b) {
; CHECK-LABEL: @neg_max_sub_ugt(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    [[TMP2:%.*]] = sub i64 0, [[TMP1]]
; CHECK-NEXT:    [[EXTRASUB:%.*]] = sub i64 [[A]], [[B]]
; CHECK-NEXT:    call void @use(i64 [[EXTRASUB]])
; CHECK-NEXT:    ret i64 [[TMP2]]
;
  %cmp = icmp ugt i64 %a, %b
  %sub = sub i64 %b, %a
  %sel = select i1 %cmp, i64 %sub ,i64 0
  %extrasub = sub i64 %a, %b
  call void @use(i64 %extrasub)
  ret i64 %sel
}

; ((b < a) ? b - a : 0) -> -usub.sat(a, b)

define i64 @neg_max_sub_ult(i64 %a, i64 %b) {
; CHECK-LABEL: @neg_max_sub_ult(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    [[TMP2:%.*]] = sub i64 0, [[TMP1]]
; CHECK-NEXT:    ret i64 [[TMP2]]
;
  %cmp = icmp ult i64 %b, %a
  %sub = sub i64 %b, %a
  %sel = select i1 %cmp, i64 %sub ,i64 0
  ret i64 %sel
}

; ((b > a) ? 0 : b - a) -> -usub.sat(a, b)

define i64 @neg_max_sub_ugt_sel_swapped(i64 %a, i64 %b) {
; CHECK-LABEL: @neg_max_sub_ugt_sel_swapped(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    [[TMP2:%.*]] = sub i64 0, [[TMP1]]
; CHECK-NEXT:    ret i64 [[TMP2]]
;
  %cmp = icmp ugt i64 %b, %a
  %sub = sub i64 %b, %a
  %sel = select i1 %cmp, i64 0 ,i64 %sub
  ret i64 %sel
}

define i64 @neg_max_sub_ugt_sel_swapped_extrause1(i64 %a, i64 %b) {
; CHECK-LABEL: @neg_max_sub_ugt_sel_swapped_extrause1(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i64 [[B:%.*]], [[A:%.*]]
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A]], i64 [[B]])
; CHECK-NEXT:    [[TMP2:%.*]] = sub i64 0, [[TMP1]]
; CHECK-NEXT:    call void @usei1(i1 [[CMP]])
; CHECK-NEXT:    ret i64 [[TMP2]]
;
  %cmp = icmp ugt i64 %b, %a
  %sub = sub i64 %b, %a
  %sel = select i1 %cmp, i64 0 ,i64 %sub
  call void @usei1(i1 %cmp)
  ret i64 %sel
}

define i64 @neg_max_sub_ugt_sel_swapped_extrause2(i64 %a, i64 %b) {
; CHECK-LABEL: @neg_max_sub_ugt_sel_swapped_extrause2(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i64 [[B:%.*]], [[A:%.*]]
; CHECK-NEXT:    [[SUB:%.*]] = sub i64 [[B]], [[A]]
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i64 0, i64 [[SUB]]
; CHECK-NEXT:    call void @use(i64 [[SUB]])
; CHECK-NEXT:    ret i64 [[SEL]]
;
  %cmp = icmp ugt i64 %b, %a
  %sub = sub i64 %b, %a
  %sel = select i1 %cmp, i64 0 ,i64 %sub
  call void @use(i64 %sub)
  ret i64 %sel
}

define i64 @neg_max_sub_ugt_sel_swapped_extrause3(i64 %a, i64 %b) {
; CHECK-LABEL: @neg_max_sub_ugt_sel_swapped_extrause3(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i64 [[B:%.*]], [[A:%.*]]
; CHECK-NEXT:    [[SUB:%.*]] = sub i64 [[B]], [[A]]
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i64 0, i64 [[SUB]]
; CHECK-NEXT:    call void @use(i64 [[SUB]])
; CHECK-NEXT:    call void @usei1(i1 [[CMP]])
; CHECK-NEXT:    ret i64 [[SEL]]
;
  %cmp = icmp ugt i64 %b, %a
  %sub = sub i64 %b, %a
  %sel = select i1 %cmp, i64 0 ,i64 %sub
  call void @use(i64 %sub)
  call void @usei1(i1 %cmp)
  ret i64 %sel
}

; ((a < b) ? 0 : b - a) -> -usub.sat(a, b)

define i64 @neg_max_sub_ult_sel_swapped(i64 %a, i64 %b) {
; CHECK-LABEL: @neg_max_sub_ult_sel_swapped(
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @llvm.usub.sat.i64(i64 [[A:%.*]], i64 [[B:%.*]])
; CHECK-NEXT:    [[TMP2:%.*]] = sub i64 0, [[TMP1]]
; CHECK-NEXT:    [[EXTRASUB:%.*]] = sub i64 [[A]], [[B]]
; CHECK-NEXT:    call void @use(i64 [[EXTRASUB]])
; CHECK-NEXT:    ret i64 [[TMP2]]
;
  %cmp = icmp ult i64 %a, %b
  %sub = sub i64 %b, %a
  %sel = select i1 %cmp, i64 0 ,i64 %sub
  %extrasub = sub i64 %a, %b
  call void @use(i64 %extrasub)
  ret i64 %sel
}

define i32 @max_sub_ugt_c1(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_c1(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i32 [[A:%.*]], 1
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -1
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ugt i32 %a, 1
  %sub = add i32 %a, -1
  %sel = select i1 %cmp, i32 %sub ,i32 0
  ret i32 %sel
}

define i32 @max_sub_ugt_c01(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_c01(
; CHECK-NEXT:    [[CMP:%.*]] = icmp eq i32 [[A:%.*]], 0
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -1
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 0, i32 [[SUB]]
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ugt i32 %a, 0
  %sub = add i32 %a, -1
  %sel = select i1 %cmp, i32 %sub ,i32 0
  ret i32 %sel
}

define i32 @max_sub_ugt_c10(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_c10(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i32 [[A:%.*]], 10
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -10
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ugt i32 %a, 10
  %sub = add i32 %a, -10
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ugt_c910(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_c910(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i32 [[A:%.*]], 9
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -10
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ugt i32 %a, 9
  %sub = add i32 %a, -10
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ugt_c1110(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_c1110(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i32 [[A:%.*]], 11
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -10
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ugt i32 %a, 11
  %sub = add i32 %a, -10
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ugt_c0(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_c0(
; CHECK-NEXT:    ret i32 0
;
  %cmp = icmp ugt i32 %a, -1
  %sub = add i32 %a, 0
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ugt_cmiss(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_cmiss(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i32 [[A:%.*]], 1
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -2
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ugt i32 %a, 1
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ult_c1(i32 %a) {
; CHECK-LABEL: @max_sub_ult_c1(
; CHECK-NEXT:    [[CMP:%.*]] = icmp eq i32 [[A:%.*]], 0
; CHECK-NEXT:    [[SEL:%.*]] = sext i1 [[CMP]] to i32
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ult i32 %a, 1
  %sub = add i32 %a, -1
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ult_c2(i32 %a) {
; CHECK-LABEL: @max_sub_ult_c2(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i32 [[A:%.*]], 2
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -2
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ult i32 %a, 2
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ult_c2_oneuseicmp(i32 %a) {
; CHECK-LABEL: @max_sub_ult_c2_oneuseicmp(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i32 [[A:%.*]], 2
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -2
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    call void @usei1(i1 [[CMP]])
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ult i32 %a, 2
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  call void @usei1(i1 %cmp)
  ret i32 %sel
}

define i32 @max_sub_ult_c2_oneusesub(i32 %a) {
; CHECK-LABEL: @max_sub_ult_c2_oneusesub(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i32 [[A:%.*]], 2
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -2
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    call void @usei32(i32 [[SUB]])
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ult i32 %a, 2
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  call void @usei32(i32 %sub)
  ret i32 %sel
}

define i32 @max_sub_ult_c32(i32 %a) {
; CHECK-LABEL: @max_sub_ult_c32(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i32 [[A:%.*]], 3
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -2
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ult i32 %a, 3
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ugt_c32(i32 %a) {
; CHECK-LABEL: @max_sub_ugt_c32(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i32 [[A:%.*]], 3
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -2
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ugt i32 3, %a
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_uge_c32(i32 %a) {
; CHECK-LABEL: @max_sub_uge_c32(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i32 [[A:%.*]], 3
; CHECK-NEXT:    [[SUB:%.*]] = add i32 [[A]], -2
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 [[SUB]], i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp uge i32 2, %a
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ult_c12(i32 %a) {
; CHECK-LABEL: @max_sub_ult_c12(
; CHECK-NEXT:    [[CMP:%.*]] = icmp eq i32 [[A:%.*]], 0
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i32 -2, i32 0
; CHECK-NEXT:    ret i32 [[SEL]]
;
  %cmp = icmp ult i32 %a, 1
  %sub = add i32 %a, -2
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

define i32 @max_sub_ult_c0(i32 %a) {
; CHECK-LABEL: @max_sub_ult_c0(
; CHECK-NEXT:    ret i32 0
;
  %cmp = icmp ult i32 %a, 0
  %sub = add i32 %a, -1
  %sel = select i1 %cmp, i32 %sub, i32 0
  ret i32 %sel
}

