%require "3.2"
%language "c++"
%locations

%define api.value.type variant
%define api.token.constructor
%define parse.error detailed

%lex-param   { ParserContext& ctx }
%parse-param { ParserContext& ctx }

%code requires {
    import ast;
    import parser_context;
}

%code provides {
    extern yy::parser::symbol_type yylex(ParserContext& ctx);
}

%token <ast::anyNode> NUMBER VAR STRLITERAL
%token PLUS MINUS STAR SLASH A L AE LE E NE COLON ASSIGN IF ELSE LPARENTHESIS RPARENTHESIS LBRACE RBRACE WHILE FUNC COMMA STRUCT DOT

%type <ast::anyNode> expr stmt block single_arg single_call_arg
%type <std::vector<ast::anyNode>> stmt_list struct_args call_args

%left PLUS MINUS
%left STAR SLASH A L AE LE E NE

%%

start:
    block { ctx.result_ = std::move($1); }
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
        std::vector<ast::anyNode> vec;
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

    // initialization var as struct
    | VAR VAR {
        auto&& structName = ($1).as<ast::Var>().data();
        auto&& var = $2;
        $$ = ast::Assign(std::move(var), ast::Struct(structName, {}), true);
    }

    // assignment
    | VAR ASSIGN { ctx.setCurrentUnit("expr", @2); } expr {
        ctx.resetCurrentUnit();
        auto&& var = $1;
        $$ = ast::Assign(std::move(var), std::move($4) /*, false */);
    }
    // initialization
    | VAR COLON ASSIGN { ctx.setCurrentUnit("expr", @3); } expr {
        ctx.resetCurrentUnit();
        auto&& var = $1;
        $$ = ast::Assign(std::move(var), std::move($5), true);
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
              std::move(ast::anyNode(ast::Struct(funcName, std::move($4)))),
              std::move($6)
          );
    }
    // function call as statement
    | VAR LPARENTHESIS call_args RPARENTHESIS {
        const std::string& funcName = $1.as<ast::Var>().data();
        $$ = ast::FuncCall(funcName, ast::anyNode(ast::Struct(funcName, std::move($3))));
    }

    | VAR DOT VAR ASSIGN { ctx.setCurrentUnit("expr", @4); } expr {
        ctx.resetCurrentUnit();
        const std::string& structName = $1.as<ast::Var>().data();
        const std::string& editableFieldName = $3.as<ast::Var>().data();

        $$ = ast::StructEditor(
            structName, 
            std::move(ast::StructField(editableFieldName, std::move($6)))
        );
    }
    ;

struct_args:
    /* empty */ { 
        $$ = std::vector<ast::anyNode>(); 
    }
    | single_arg { 
        std::vector<ast::anyNode> v;
        v.push_back(std::move($1));
        $$ = std::move(v); 
    }
    | struct_args COMMA single_arg {
        $1.push_back(std::move($3));
        $$ = std::move($1);
    }
    ;

single_arg:
    VAR ASSIGN { ctx.setCurrentUnit("expr", @2); } expr {
        ctx.resetCurrentUnit();
        const std::string& name = $1.as<ast::Var>().data();
        $$ = ast::anyNode(ast::StructField(name, std::move($4)));
    }
    | VAR {
        const std::string& name = $1.as<ast::Var>().data();
        $$ = ast::anyNode(ast::StructField(name));
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
        $$ = ast::FuncCall(funcName, ast::anyNode(ast::Struct(funcName, std::move($3))));
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
        $$ = std::vector<ast::anyNode>(); 
    }
    | single_call_arg { 
        std::vector<ast::anyNode> v;
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
        $$ = ast::anyNode(ast::StructField("arg", std::move($1)));
    }
    ;


%%

void yy::parser::error(const location_type& loc, const std::string& msg)
{
    std::cerr << "Syntax error: \n";

    ctx.check();

    std::cerr << "Parse error at " 
            << loc.begin.line << ":" << loc.begin.column 
            << ": " << msg << std::endl;        
}