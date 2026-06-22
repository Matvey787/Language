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

%token <ast::AnyNode> NUMBER IDENTIFIER
%token PLUS MINUS STAR SLASH A L AE LE E NE ASSIGN IF ELSE LPARENTHESIS RPARENTHESIS LBRACE RBRACE WHILE FUNC COMMA

%type <ast::AnyNode> expr stmt block
%type <std::vector<ast::AnyNode>> stmt_list func_args

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
    IDENTIFIER ASSIGN expr {
        $$ = ast::Assign(std::move($1), std::move($3));
    }
    | expr {
        $$ = std::move($1);
    }
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
    // function definition
    | FUNC IDENTIFIER LPARENTHESIS func_args RPARENTHESIS block {
          $$ = ast::Func(
              std::move($2.as<ast::Lit<std::string>>().data()),
              std::move($4),
              std::move($6)
          );
    }
    // function call
    | IDENTIFIER LPARENTHESIS func_args RPARENTHESIS {
        
    }
    ;

func_args:
      func_args COMMA IDENTIFIER {
          $1.emplace_back(std::move($3));
          $$ = std::move($1);
      }
    | IDENTIFIER {
          $$ = std::vector<ast::AnyNode>{std::move($1)};
      }
    ;


expr:
    expr PLUS expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::Add);
    }
    | expr MINUS expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::Sub);
    }
    | expr STAR expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::Mul);
    }
    | expr SLASH expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::Div);
    }
    | expr A expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::A);
    }
    | expr AE expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::AE);
    }
    | expr L expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::L);
    }
    | expr LE expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::LE);
    }
    | expr E expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::E);
    }
    | expr NE expr {
        $$ = ast::BinOp(std::move($1), std::move($3), ast::BinOp::binOpType::NE);
    }
    | NUMBER {
        $$ = std::move($1);
    }
    | IDENTIFIER {
        $$ = std::move($1);
    }
    ;

%%

void yy::parser::error(const std::string& msg) {
    std::cerr << "Parse error: " << msg << std::endl;
}