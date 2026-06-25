; ModuleID = 'main'
source_filename = "main"
target triple = "x86_64-pc-linux-gnu"

define i32 @main() {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 1, ptr %a, align 4
  store i32 5, ptr %b, align 4
  ret i32 0
}

define i32 @poop(i32 %0, i32 %1) {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 1123, ptr %a, align 4
  %a1 = load i32, ptr %a, align 4
  %b2 = load i32, ptr %b, align 4
  %add = add i32 %a1, %b2
  store i32 %add, ptr %c, align 4
  ret i32 0
}
