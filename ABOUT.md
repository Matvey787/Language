# About error handling

All semantic errors are handled using the `ErrorHandlerExt` extension (this is one of the base extension classes for the `anyNode` - type-erasure node). See [error_handler.cppm](compiler/front/ast/src/extensions/error_handler.cppm), [ir_first_pass.cppm](compiler/front/src/llvm/ir_first_pass.cppm) (semantic pass) and [ir_second_pass.cppm](compiler/front/src/llvm/ir_second_pass.cppm) (semantic pass).

| # | Message | Type | Where |
|---|---------|------|-------|
| 1 | `use of undeclared identifier '{x}'` | ERROR | Assignment |
| 2 | `use of undeclared struct '{S}'` | ERROR | Struct init |
| 3 | `use of undeclared (struct) identifier '{S}'` | ERROR | Field access |
| 4 | `no member named '{f}' in '{S}'` | ERROR | Field access |
| 5 | `use of undeclared function '{f}'` | ERROR | Function call |
| 6 | `redefinition of struct '{S}'` | ERROR | Struct definition |
| 7 | `function definition is not allowed in non-global scope` | ERROR | Function definition |
| 8 | `too many arguments to function '{f}', expected {n} but got {m}` | WARNING | Function call |
| 9 | `use of uninitialised field '{f}' in struct '{S}'` | WARNING | Field access |
| 10 | `division by zero` | WARNING | Binary expression |
| 11 | `struct definition is here` | NOTE | Field access |
| 12 | `redefinition of '{name}'` | ERROR | Variable init |
| 13 | `redefinition of function '{name}'` | ERROR | Function definition |
| 14 | `redefinition of field '{f}' in struct '{S}'` | ERROR | Struct definition |
| 15 | `cannot assign value of type '{type2}' to variable '{name}' of type '{type1}'` | ERROR | Assignment |
| 16 | `invalid operands to binary expression ({type1} op {type2})` | ERROR | Binary expression |
| 17 | `cannot compare values of types '{type1}' and '{type2}'` | ERROR | Comparison |
| 18 | `too few arguments to function '{f}', expected {n} but got {m}` | ERROR | Function call |
| 19 | `struct name '{name}' cannot be used as an expression` | ERROR | Expression |
| 20 | `local variable '{name}' shadows a previously declared variable` | WARNING | Variable init |
| 21 | `condition is always constant in while loop` | WARNING | While loop |
| 22 | `condition is always constant in if statement` | WARNING | If statement |
