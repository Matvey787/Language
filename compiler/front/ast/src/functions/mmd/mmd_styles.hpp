#define compileStr static constexpr const char*

// STYLE_M(NodeType, fill, stroke, color, borderType)

STYLE_M(Lit<int>, "#ffffff", "#0000ff", "#000000", "()")
STYLE_M(Lit<std::string>, "#ffffff", "#0000ff", "#000000", "()")
STYLE_M(Var, "#ffffff", "#ff0000", "#000000", "()")
STYLE_M(BinOp, "#d3ff5b", "#d3ff5b", "#000000", "[]")
STYLE_M(Assign, "#d3ff5b", "#d3ff5b", "#000000", "[]")
STYLE_M(Block, "#5a9d36", "#5a9d36", "#000000", "[]")
STYLE_M(IfElse, "#d3ff5b", "#d3ff5b", "#000000", "[]")
STYLE_M(While, "#d3ff5b", "#d3ff5b", "#000000", "[]")
STYLE_M(Func, "#d3ff5b", "#d3ff5b", "#000000", "[]")
