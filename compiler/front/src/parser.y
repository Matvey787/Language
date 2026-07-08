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

    #include <bits/stringfwd.h>
}

%code provides {
    extern yy::parser::symbol_type yylex(ParserContext& ctx);
}

%token <ast::anyNode> NUMBER VAR STRLITERAL
%token PLUS MINUS STAR SLASH A L AE LE E NE COLON ASSIGN IF ELSE LPARENTHESIS RPARENTHESIS LBRACE RBRACE WHILE FUNC COMMA STRUCT DOT RETURN SKIP

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

        auto&& structVar($1);
        auto&& tokenWidth = (@1).end.line - (@1).begin.line;

        ast::LocationExt<ast::anyNode> location(
            (@1).begin.line, 
            (@1).begin.column,
            tokenWidth
        );

        ast::ErrorHandlerExt<ast::anyNode> errorHandler(
            ctx.getSourceFile()
        );

        auto&& structName = ($1).as<ast::Var>().data();
        auto&& var($2);
        $$ = ast::anyNode(
            ast::Assign(
                std::move(var),
                ast::anyNode(
                    ast::Struct(structName, std::move($6)),
                    ast::LocationExt<ast::anyNode>((@1).begin.line, (@1).begin.column, tokenWidth),
                    ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
                ),
                true
            ),
            std::move(location),
            std::move(errorHandler)
        );
    }

    // initialization var as struct
    | VAR VAR {
        auto&& structVar($1);
        auto&& tokenWidth = (@1).end.line - (@1).begin.line;

        ast::LocationExt<ast::anyNode> location(
            (@1).begin.line, 
            (@1).begin.column,
            tokenWidth
        );

        ast::ErrorHandlerExt<ast::anyNode> errorHandler(
            ctx.getSourceFile()
        );

        auto&& structName = ($1).as<ast::Var>().data();
        auto&& var($2);

        $$ = ast::Assign(
            std::move(var),
            ast::anyNode(
                ast::Struct(structName, {}),
                std::move(location), 
                std::move(errorHandler) 
            ),
            true
        );
    }

    // assignment
    | VAR ASSIGN { ctx.setCurrentUnit("expr", @2); } expr {
        ctx.resetCurrentUnit();

        auto&& var($1);

        ast::LocationExt<ast::anyNode> location(
            (@1).begin.line, 
            (@1).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> errorHandler(
            ctx.getSourceFile()
        );

        $$ = ast::anyNode(
            ast::Assign(
                ast::anyNode(
                    var.asMove<ast::Var>(), 
                    std::move(location), 
                    std::move(errorHandler)
                ), 
                std::move($4) /*, false */
            ),
            ast::LocationExt<ast::anyNode>(
                (@1).begin.line,
                (@1).begin.column
            ),
            ast::ErrorHandlerExt<ast::anyNode>(
                ctx.getSourceFile()
            )
        );
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

        auto&& width = (@1).end.column - (@1).begin.column;
        auto&& height = (@5).end.line - (@1).begin.line + 1;
        auto&& structName = $2.as<ast::Var>().data();

        ast::LocationExt<ast::anyNode> struct_location(
            (@1).begin.line, 
            (@1).begin.column,
            width,
            height
        );

        ast::ErrorHandlerExt<ast::anyNode> struct_errorHandler(
            ctx.getSourceFile()
        );

        $$ = ast::anyNode(
            ast::Struct(structName, std::move($4)),
            struct_location,
            struct_errorHandler
        );
        
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
        auto&& width = (@1).end.column - (@1).begin.column;

        ast::LocationExt<ast::anyNode> func_call_location(
            (@1).begin.line, 
            (@1).begin.column,
            width
        );

        ast::ErrorHandlerExt<ast::anyNode> func_call_errorHandler(
            ctx.getSourceFile()
        );

        $$ = ast::anyNode(
            ast::FuncCall(funcName, ast::anyNode(ast::Struct(funcName, std::move($3)))),
            func_call_location,
            func_call_errorHandler
        );
    }
    | RETURN expr {
        $$ = ast::Return(std::move($2));
    }

    | VAR DOT VAR ASSIGN { ctx.setCurrentUnit("expr", @4); } expr {
        ctx.resetCurrentUnit();

        auto&& structName = $1.as<ast::Var>().data();
        auto&& editableFieldName = $3.as<ast::Var>().data();

        ast::LocationExt<ast::anyNode> struct_var_location(
            (@1).begin.line, 
            (@1).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> struct_var_errorHandler(
            ctx.getSourceFile()
        );

        ast::LocationExt<ast::anyNode> field_location(
            (@3).begin.line, 
            (@3).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> field_errorHandler(
            ctx.getSourceFile()
        );

        $$ = ast::anyNode(
            ast::StructEditor(
                structName, 
                ast::anyNode(
                    ast::StructField(editableFieldName, std::move($6)),
                    std::move(field_location),
                    std::move(field_errorHandler)
                )
            ),
            std::move(struct_var_location),
            std::move(struct_var_errorHandler)
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
        ast::LocationExt<ast::anyNode> field_location(
            (@1).begin.line, 
            (@1).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> field_errorHandler(
            ctx.getSourceFile()
        );

        const std::string& name = $1.as<ast::Var>().data();
        $$ = ast::anyNode(
            ast::StructField(name),
            field_location,
            field_errorHandler);
    }
    ;


expr:
    expr PLUS expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::ADD),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr MINUS expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::SUB),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr STAR expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::MUL),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr SLASH expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::DIV),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr A expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::A),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr AE expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::AE),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr L expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::L),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr LE expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::LE),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr E expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::E),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    | expr NE expr {
        $$ = ast::anyNode(
            ast::BinOp(std::move($1), std::move($3), ast::BinOp::BinOpType::NE),
            ast::LocationExt<ast::anyNode>((@2).begin.line, (@2).begin.column),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        );
    }
    // function call
    | VAR LPARENTHESIS call_args RPARENTHESIS {
        const std::string& funcName = $1.as<ast::Var>().data();
        auto&& width = (@1).end.column - (@1).begin.column;

        ast::LocationExt<ast::anyNode> func_call_location(
            (@1).begin.line, 
            (@1).begin.column,
            width
        );

        ast::ErrorHandlerExt<ast::anyNode> func_call_errorHandler(
            ctx.getSourceFile()
        );

        $$ = ast::anyNode(
            ast::FuncCall(funcName, ast::anyNode(ast::Struct(funcName, std::move($3)))),
            func_call_location,
            func_call_errorHandler
        );
    }
    // struct field
    | VAR DOT VAR {
        auto&& structName = $1.as<ast::Var>().data();
        auto&& editableFieldName = $3.as<ast::Var>().data();

        ast::LocationExt<ast::anyNode> struct_var_location(
            (@1).begin.line, 
            (@1).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> struct_var_errorHandler(
            ctx.getSourceFile()
        );

        ast::LocationExt<ast::anyNode> field_location(
            (@3).begin.line, 
            (@3).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> field_errorHandler(
            ctx.getSourceFile()
        );


        $$ = ast::anyNode(
            ast::StructEditor(
                structName, 
                ast::anyNode(
                    ast::StructField(editableFieldName),
                    std::move(field_location),
                    std::move(field_errorHandler)
                )
            ),
            std::move(struct_var_location),
            std::move(struct_var_errorHandler)
        );
         
    }
    | STRLITERAL {
        $$ = std::move($1);
    }
    | VAR {
        auto&& location = ast::LocationExt<ast::anyNode>(
            (@1).begin.line,
            (@1).begin.column
        );
        auto&& errorHandler = ast::ErrorHandlerExt<ast::anyNode>(
            ctx.getSourceFile()
        );
        $$ = ast::anyNode(
            $1.asMove<ast::Var>(),
            std::move(location),
            std::move(errorHandler)
        );
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
        ast::LocationExt<ast::anyNode> field_location(
            (@1).begin.line,
            (@1).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> field_errorHandler(
            ctx.getSourceFile()
        );

        $$ = ast::anyNode(
            ast::StructField("arg", std::move($1)),
            field_location,
            field_errorHandler);
    }
    | SKIP {
        ast::LocationExt<ast::anyNode> field_location(
            (@1).begin.line,
            (@1).begin.column
        );

        ast::ErrorHandlerExt<ast::anyNode> field_errorHandler(
            ctx.getSourceFile()
        );

        $$ = ast::anyNode(
            ast::StructField("arg"),
            field_location,
            field_errorHandler);
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
