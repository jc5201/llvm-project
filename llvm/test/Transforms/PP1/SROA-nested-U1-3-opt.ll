; ModuleID = 'SROA-nested-U1-3.ll'
source_filename = "SROA-nested-U1-3.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.second = type { i32, %struct.simple }
%struct.simple = type { i32, i32 }

; Function Attrs: noinline norecurse nounwind uwtable
define dso_local i32 @main() #0 {
entry:
  %s = alloca %struct.second, align 4
  %d = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %a = getelementptr inbounds %struct.simple, %struct.simple* %d, i32 0, i32 0
  %0 = load i32, i32* %a, align 4
  %d1 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %b = getelementptr inbounds %struct.simple, %struct.simple* %d1, i32 0, i32 1
  %1 = load i32, i32* %b, align 4
  %add = add nsw i32 %0, %1
  %c = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 0
  %2 = load i32, i32* %c, align 4
  %add2 = add nsw i32 %add, %2
  ret i32 %add2
}

attributes #0 = { noinline norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0 (https://github.com/llvm/llvm-project.git 2476548dd5ff243a9821183903117e1f3c850066)"}
