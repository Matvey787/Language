; ModuleID = 'main'
source_filename = "main"
target triple = "x86_64-pc-linux-gnu"

%car = type { i32, i32, i32, i32 }

define i32 @main() {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %0 = alloca %car, align 8
  store i32 1, ptr %a, align 4
  store i32 5, ptr %b, align 4
  %1 = getelementptr %car, ptr %0, i32 0, i32 0
  store i32 10, ptr %1, align 4
  ret i32 0
}
