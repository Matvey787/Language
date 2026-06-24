; ModuleID = 'main'
source_filename = "main"
target triple = "x86_64-pc-linux-gnu"

define i32 @main() {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %a1 = alloca i32, align 4
  %a2 = alloca i32, align 4
  %a3 = alloca i32, align 4
  %a4 = alloca i32, align 4
  store i32 1, ptr %a, align 4
  store i32 5, ptr %b, align 4
  %a5 = load i32, ptr %a, align 4
  %b6 = load i32, ptr %b, align 4
  %l = icmp slt i32 %a5, %b6
  br i1 %l, label %if, label %else

if:                                               ; preds = %entry
  store i32 100, ptr %a1, align 4
  br label %endif

else:                                             ; preds = %entry
  store i32 200, ptr %a2, align 4
  br label %endif

endif:                                            ; preds = %else, %if
  %0 = phi ptr [ %a1, %if ], [ %a2, %else ]
  %a7 = load i32, ptr %a, align 4
  %b8 = load i32, ptr %b, align 4
  %l9 = icmp slt i32 %a7, %b8
  br i1 %l9, label %if10, label %else11

if10:                                             ; preds = %endif
  store i32 300, ptr %a3, align 4
  br label %endif12

else11:                                           ; preds = %endif
  store i32 400, ptr %a4, align 4
  br label %endif12

endif12:                                          ; preds = %else11, %if10
  %1 = phi ptr [ %a3, %if10 ], [ %a4, %else11 ]
  ret i32 0
}
