; ModuleID = 'SROA-nested-U2-1.bc'
source_filename = "SROA-nested-U2-1.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.second = type { i32, %struct.simple }
%struct.simple = type { i32, i32 }

; Function Attrs: noinline norecurse nounwind uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %s = alloca %struct.second, align 4
  %t = alloca %struct.simple, align 4
  store i32 0, i32* %retval, align 4
  %c = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 0
  store i32 1, i32* %c, align 4
  %d = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %a = getelementptr inbounds %struct.simple, %struct.simple* %d, i32 0, i32 0
  store i32 2, i32* %a, align 4
  %d1 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %b = getelementptr inbounds %struct.simple, %struct.simple* %d1, i32 0, i32 1
  store i32 3, i32* %b, align 4
  %a2 = getelementptr inbounds %struct.simple, %struct.simple* %t, i32 0, i32 0
  store i32 4, i32* %a2, align 4
  %b3 = getelementptr inbounds %struct.simple, %struct.simple* %t, i32 0, i32 1
  store i32 5, i32* %b3, align 4
  %d4 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %add.ptr = getelementptr inbounds %struct.simple, %struct.simple* %d4, i64 1
  %cmp = icmp eq %struct.simple* %t, %add.ptr
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  store i32 1, i32* %retval, align 4
  br label %return

if.end:                                           ; preds = %entry
  %d5 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %a6 = getelementptr inbounds %struct.simple, %struct.simple* %d5, i32 0, i32 0
  %0 = load i32, i32* %a6, align 4
  %d7 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 1
  %b8 = getelementptr inbounds %struct.simple, %struct.simple* %d7, i32 0, i32 1
  %1 = load i32, i32* %b8, align 4
  %add = add nsw i32 %0, %1
  %c9 = getelementptr inbounds %struct.second, %struct.second* %s, i32 0, i32 0
  %2 = load i32, i32* %c9, align 4
  %add10 = add nsw i32 %add, %2
  store i32 %add10, i32* %retval, align 4
  br label %return

return:                                           ; preds = %if.end, %if.then
  %3 = load i32, i32* %retval, align 4
  ret i32 %3
}

attributes #0 = { noinline norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0 (https://github.com/llvm/llvm-project.git 2476548dd5ff243a9821183903117e1f3c850066)"}
