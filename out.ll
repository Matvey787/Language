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
