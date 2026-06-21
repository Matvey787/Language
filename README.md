# ParaCL

```mermaid
graph TD

%% Available styles
classDef Lit_int_ fill:#ffffff, stroke:#0000ff, color:#000000;
classDef Lit_std::string_ fill:#ffffff, stroke:#ff0000, color:#000000;
classDef BinOp fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef Assign fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef Block fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef IfElse fill:#d3ff5b, stroke:#d3ff5b, color:#000000;

%% AST tree
1[block]:::Block
4(a):::Lit_std::string_
5(10):::Lit_int_
3[L]:::BinOp
3 --> 4
3 --> 5
6[block]:::Block
8(x):::Lit_std::string_
9(5):::Lit_int_
7[Add]:::BinOp
7 --> 8
7 --> 9
6 --> 7
11(y):::Lit_std::string_
12(1):::Lit_int_
10[Sub]:::BinOp
10 --> 11
10 --> 12
6 --> 10
2[if_else]:::IfElse
2 --> 3
2 --> 6
13[block]:::Block
15(b):::Lit_std::string_
16(0):::Lit_int_
14[assignment]:::Assign
14 --> 15
14 --> 16
13 --> 14
2 --> 13
1 --> 2
```
