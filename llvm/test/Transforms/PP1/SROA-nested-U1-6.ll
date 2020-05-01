; ModuleID = 'SROA-nested-U1-6.bc'
source_filename = "SROA-nested-U1-6.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.second = type { i8, %struct.simple }
%struct.simple = type { i32, double }

; Function Attrs: noinline norecurse nounwind uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %s = alloca %struct.second, align 8
  store i32 0, i32* %retval, align 4
  %c = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 0
  store i8 1, i8* %c, align 8
  %d = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %a = getelementptr inbounds %struct.simple, %struct.simple* %d, i32 0, i32 0
  store i32 2, i32* %a, align 8
  %d1 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %b = getelementptr inbounds %struct.simple, %struct.simple* %d1, i32 0, i32 1
  store double 3.000000e+00, double* %b, align 8
  %d2 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %a3 = getelementptr inbounds %struct.simple, %struct.simple* %d2, i32 0, i32 0
  %0 = load i32, i32* %a3, align 8
  %conv = sitofp i32 %0 to double
  %d4 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %b5 = getelementptr inbounds %struct.simple, %struct.simple* %d4, i32 0, i32 1
  %1 = load double, double* %b5, align 8
  %add = fadd double %conv, %1
  %c6 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 0
  %2 = load i8, i8* %c6, align 8
  %conv7 = sext i8 %2 to i32
  %conv8 = sitofp i32 %conv7 to double
  %add9 = fadd double %add, %conv8
  %conv10 = fptosi double %add9 to i32
  ret i32 %conv10
}

attributes #0 = { noinline norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0 (https://github.com/llvm/llvm-project.git 2476548dd5ff243a9821183903117e1f3c850066)"}
