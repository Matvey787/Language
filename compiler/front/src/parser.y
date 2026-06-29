%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor
%define parse.error verbose
%parse-param {ast::AnyNode& result}

%code requires {
    import ast;
}

%code provides {
    extern yy::parser::symbol_type yylex();
}

%token <ast::AnyNode> NUMBER VAR STRLITERAL
%token PLUS MINUS STAR SLASH A L AE LE E NE COLON ASSIGN IF ELSE LPARENTHESIS RPARENTHESIS LBRACE RBRACE WHILE FUNC COMMA STRUCT DOT

%type <ast::AnyNode> expr stmt block single_arg single_call_arg
%type <std::vector<ast::AnyNode>> stmt_list struct_args call_args

%left PLUS MINUS
%left STAR SLASH A L AE LE E NE

%%

start:
    block { result = std::move($1); }
;

block:
    LBRACE stmt_list RBRACE {
        $$ = ast::Block(std::move($2));
    }
;

stmt_list:
    stmt_list stmt {
        $1.push_back(std::move($2));
        $$ = std::move($1);
    }
    | stmt {
        std::vector<ast::AnyNode> vec;
        vec.push_back(std::move($1));
        $$ = std::move(vec);
    }
    ;

stmt:
    // initialization var as struct
    VAR VAR COLON ASSIGN LBRACE call_args RBRACE {
        auto&& structName = ($1).as<ast::Var>().data();
        auto&& var = $2;
        $$ = ast::Assign(std::move(var), ast::Struct(structName, std::move($6)), true);
    }
    |
    // assignment
    VAR ASSIGN expr {
        auto&& var = $1;
        $$ = ast::Assign(std::move(var), std::move($3) /*, false */);
    }
    // initialization
    | VAR COLON ASSIGN expr {
        auto&& var = $1;
        $$ = ast::Assign(std::move(var), std::move($4), true);
    }
    /* | expr {
        $$ = std::move($1);
    } */
    | block {
        $$ = std::move($1);
    }
    | IF LPARENTHESIS expr RPARENTHESIS block {
        $$ = ast::IfElse(std::move($3), std::move($5));
    }
    | IF LPARENTHESIS expr RPARENTHESIS block ELSE block {
        $$ = ast::IfElse(std::move($3), std::move($5), std::move($7));
    }
    | WHILE LPARENTHESIS expr RPARENTHESIS block {
        $$ = ast::While(std::move($3), std::move($5));
    }
    // struct defenition
    | STRUCT VAR LBRACE struct_args RBRACE {
        const std::string& structName = $2.as<ast::Var>().data();
        $$ = ast::Struct(structName, std::move($4));
    }
    // function definition
    | FUNC VAR LPARENTHESIS struct_args RPARENTHESIS block {
        const std::string& funcName = $2.as<ast::Var>().data();
          $$ = ast::Func(
              funcName,
              std::move(ast::AnyNode(ast::Struct(funcName, std::move($4)))),
              std::move($6)
          );
    }
    // function call as statement
    | VAR LPARENTHESIS call_args RPARENTHESIS {
        const std::string& funcName = $1.as<ast::Var>().data();
        $$ = ast::FuncCall(funcName, ast::AnyNode(ast::Struct(funcName, std::move($3))));
    }

    | VAR DOT VAR ASSIGN expr {
        const std::string& structName = $1.as<ast::Var>().data();
        const std::string& editableFieldName = $3.as<ast::Var>().data();

        $$ = ast::StructEditor(
            structName, 
            std::move(ast::StructField(editableFieldName, std::move($5)))
        );
    }
    ;

struct_args:
    /* empty */ { 
        $$ = std::vector<ast::AnyNode>(); 
    }
    | single_arg { 
        std::vector<ast::AnyNode> v;
        v.push_back(std::move($1));
        $$ = std::move(v); 
    }
    | struct_args COMMA single_arg {
        $1.push_back(std::move($3));
        $$ = std::move($1);
    }
    ;

single_arg:
    VAR ASSIGN expr {
        const std::string& name = $1.as<ast::Var>().data();
        $$ = ast::AnyNode(ast::StructField(name, std::move($3)));
    }
    | VAR {
        const std::string& name = $1.as<ast::Var>().data();
        $$ = ast::AnyNode(ast::StructField(name));
    }
    ;


expr:
    expr PLUS expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::ADD);
    }
    | expr MINUS expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::SUB);
    }
    | expr STAR expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::MUL);
    }
    | expr SLASH expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::DIV);
    }
    | expr A expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::A);
    }
    | expr AE expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::AE);
    }
    | expr L expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::L);
    }
    | expr LE expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::LE);
    }
    | expr E expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::E);
    }
    | expr NE expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::NE);
    }
    // function call
    | VAR LPARENTHESIS call_args RPARENTHESIS {
        auto&& funcName = ($1).as<ast::Var>().data();
        $$ = ast::FuncCall(funcName, ast::AnyNode(ast::Struct(funcName, std::move($3))));
    }
    | STRLITERAL {
        $$ = std::move($1);
    }
    | VAR {
        $$ = std::move($1);
    }
    | NUMBER {
        $$ = std::move($1);
    }
    ;

call_args:
    /* empty */ { 
        $$ = std::vector<ast::AnyNode>(); 
    }
    | single_call_arg { 
        std::vector<ast::AnyNode> v;
        v.push_back(std::move($1));
        $$ = std::move(v); 
    }
    | call_args COMMA single_call_arg {
        $1.push_back(std::move($3));
        $$ = std::move($1);
    }
    ;

single_call_arg:
    expr {
        $$ = ast::AnyNode(ast::StructField("arg", std::move($1)));
    }
    ;


%%

void yy::parser::error(const std::string& msg) {
    std::cerr << "Parse error: " << msg << std::endl;
}