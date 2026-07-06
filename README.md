# ParaCL

## Example

```c
{
    struct Max
    {
        Sebeleff1 = 100,
        Sebeleff2 = 900 * 200 - 123 / 100000
    }

    Max.Sebeleff1 = 450

    func poop(a = 10, b, c = 3, d)
    {
        z = a + b * c - d
    }

    poop(1, 2, 3, 4)

    {
        a = 10
        {
            if (a < 10)
            {
                x = 9 + 11
                a = x * 1 + 300
                x = a != 23
            }
            while (a < 100)
            {
                a = a + 1
            }
        }
    }


}
```

```mermaid
graph TD

%% Available styles
classDef Lit_int_ fill:#ffffff, stroke:#0000ff, color:#000000;
classDef Lit_std__string_ fill:#ffffff, stroke:#ff0000, color:#000000;
classDef BinOp fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef Assign fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef Block fill:#5a9d36, stroke:#5a9d36, color:#000000;
classDef IfElse fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef While fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef default fill:#FFFFFF, stroke:#000000, color:#000000;
classDef default fill:#FFFFFF, stroke:#000000, color:#000000;
classDef default fill:#FFFFFF, stroke:#000000, color:#000000;
classDef Func fill:#d3ff5b, stroke:#d3ff5b, color:#000000;
classDef default fill:#FFFFFF, stroke:#000000, color:#000000;

%% AST tree
1[block]:::Block
2(Struct Max):::default
3(Sebeleff1):::default
4(100):::Lit_int_
3 --> 4
2 --> 3
5(Sebeleff2):::default
8(900):::Lit_int_
9(200):::Lit_int_
7[Mul]:::BinOp
7 --> 8
7 --> 9
11(123):::Lit_int_
12(100000):::Lit_int_
10[Div]:::BinOp
10 --> 11
10 --> 12
6[Sub]:::BinOp
6 --> 7
6 --> 10
5 --> 6
2 --> 5
1 --> 2
13(Edit Max):::default
14(Sebeleff1):::default
15(450):::Lit_int_
14 --> 15
13 --> 14
1 --> 13
16[Func poop]:::Func
17(Struct poop):::default
18(a):::default
19(10):::Lit_int_
18 --> 19
17 --> 18
20(b):::default
17 --> 20
21(c):::default
22(3):::Lit_int_
21 --> 22
17 --> 21
23(d):::default
17 --> 23
16 --> 17
24[block]:::Block
26(z):::Lit_std__string_
29(a):::Lit_std__string_
31(b):::Lit_std__string_
32(c):::Lit_std__string_
30[Mul]:::BinOp
30 --> 31
30 --> 32
28[Add]:::BinOp
28 --> 29
28 --> 30
33(d):::Lit_std__string_
27[Sub]:::BinOp
27 --> 28
27 --> 33
25[assignment]:::Assign
25 --> 26
25 --> 27
24 --> 25
16 --> 24
1 --> 16
34[Call poop]:::Func
35(Struct poop):::default
36(arg):::default
37(1):::Lit_int_
36 --> 37
35 --> 36
38(arg):::default
39(2):::Lit_int_
38 --> 39
35 --> 38
40(arg):::default
41(3):::Lit_int_
40 --> 41
35 --> 40
42(arg):::default
43(4):::Lit_int_
42 --> 43
35 --> 42
34 --> 35
1 --> 34
44[block]:::Block
46(a):::Lit_std__string_
47(10):::Lit_int_
45[assignment]:::Assign
45 --> 46
45 --> 47
44 --> 45
48[block]:::Block
51(a):::Lit_std__string_
52(10):::Lit_int_
50[L]:::BinOp
50 --> 51
50 --> 52
53[block]:::Block
55(x):::Lit_std__string_
57(9):::Lit_int_
58(11):::Lit_int_
56[Add]:::BinOp
56 --> 57
56 --> 58
54[assignment]:::Assign
54 --> 55
54 --> 56
53 --> 54
60(a):::Lit_std__string_
63(x):::Lit_std__string_
64(1):::Lit_int_
62[Mul]:::BinOp
62 --> 63
62 --> 64
65(300):::Lit_int_
61[Add]:::BinOp
61 --> 62
61 --> 65
59[assignment]:::Assign
59 --> 60
59 --> 61
53 --> 59
67(x):::Lit_std__string_
69(a):::Lit_std__string_
70(23):::Lit_int_
68[NE]:::BinOp
68 --> 69
68 --> 70
66[assignment]:::Assign
66 --> 67
66 --> 68
53 --> 66
49[if_else]:::IfElse
49 --> 50
49 --> 53
48 --> 49
73(a):::Lit_std__string_
74(100):::Lit_int_
72[L]:::BinOp
72 --> 73
72 --> 74
75[block]:::Block
77(a):::Lit_std__string_
79(a):::Lit_std__string_
80(1):::Lit_int_
78[Add]:::BinOp
78 --> 79
78 --> 80
76[assignment]:::Assign
76 --> 77
76 --> 78
75 --> 76
71[while]:::While
71 --> 72
71 --> 75
48 --> 71
44 --> 48
1 --> 44
```


For contributors, or to find out how everything works behind the scenes, see: [ABOUT.md](ABOUT.md)
