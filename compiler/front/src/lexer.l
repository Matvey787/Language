%{
    #include "parser.tab.hh"
    import parser_context;
    
    #define YY_DECL yy::parser::symbol_type yylex(ParserContext& ctx)
    #define YY_USER_ACTION ctx.loc_.begin = ctx.loc_.end; ctx.loc_.columns(yyleng);

    #define yyterminate() \
    do { \
        if (ctx.loc_.begin.column == 1 && ctx.loc_.begin.line > 1)  \
        {                                                           \
            ctx.loc_.begin.line--;                                  \
            ctx.loc_.begin.column--;                                \
        }                                                           \
        ctx.loc_.end = ctx.loc_.begin;                              \
        return yy::parser::make_YYEOF(ctx.loc_);                    \
    } while(0)

%}

%option noyywrap

%%

"if" { return yy::parser::make_IF(ctx.loc_); }
"else" { return yy::parser::make_ELSE(ctx.loc_); }
"while" { return yy::parser::make_WHILE(ctx.loc_); }
"func" { return yy::parser::make_FUNC(ctx.loc_); }
"struct" { return yy::parser::make_STRUCT(ctx.loc_); }

"#" { 
    // однострочный комментарий — пропустить до конца строки
    int c;
    while ((c = yyinput()) != '\n' && c != EOF);
    if (c == '\n') {
        ctx.loc_.lines(1);
        ctx.loc_.step();
    }
}


\"[^\"]*\" {
    std::string s;
    s.reserve(yyleng - 2);
    for (const char* p = yytext + 1; p < yytext + yyleng - 1; ++p)
    {
        if (*p == '\\' && p + 1 < yytext + yyleng - 1)
        {
            ++p;
            switch (*p)
            {
            case 'n':  s += '\n'; break;
            case 't':  s += '\t'; break;
            case 'r':  s += '\r'; break;
            case '\\': s += '\\'; break;
            case '"':  s += '"';  break;
            case '0':  s += '\0'; break;
            default:   s += '\\'; s += *p; break;
            }
        }
        else
        {
            s += *p;
        }
    }

    return yy::parser::make_STRLITERAL(
        ast::Lit<std::string>(std::move(s)),
        ctx.loc_
    );
}

[a-zA-Z_][a-zA-Z0-9_]* {
    return yy::parser::make_VAR(
        ast::anyNode(
            ast::Var(std::move(std::string(yytext))),
            ast::LocationExt<ast::anyNode>(
                ctx.loc_.begin.line,
                ctx.loc_.begin.column
            ),
            ast::ErrorHandlerExt<ast::anyNode>(ctx.getSourceFile())
        ),
        ctx.loc_
    );
}

[0-9]+ {
    return yy::parser::make_NUMBER(
        ast::anyNode(ast::Lit<int>(std::atoi(yytext))),
        ctx.loc_
    );
}

"(" {
    // std::cout << "here\n";
    ctx.pushBrace(ctx.loc_);

    return yy::parser::make_LPARENTHESIS(ctx.loc_); 
}
")" {
    ctx.popBrace();

    return yy::parser::make_RPARENTHESIS(ctx.loc_);
}

"{" {
    ctx.pushBrace(ctx.loc_);

    return yy::parser::make_LBRACE(ctx.loc_); 
}
"}" {
    ctx.popBrace();

    return yy::parser::make_RBRACE(ctx.loc_);
}

">" { return yy::parser::make_A(ctx.loc_); }
">=" { return yy::parser::make_AE(ctx.loc_); }
"<" { return yy::parser::make_L(ctx.loc_); }
"<=" { return yy::parser::make_LE(ctx.loc_); }
"==" { return yy::parser::make_E(ctx.loc_); }
"!=" { return yy::parser::make_NE(ctx.loc_); }

"=" { return yy::parser::make_ASSIGN(ctx.loc_); }
"+" { return yy::parser::make_PLUS(ctx.loc_); }
"-" { return yy::parser::make_MINUS(ctx.loc_); }
"*" { return yy::parser::make_STAR(ctx.loc_); }
"/" { return yy::parser::make_SLASH(ctx.loc_); }

"," { return yy::parser::make_COMMA(ctx.loc_); }
"." { return yy::parser::make_DOT(ctx.loc_); }
":" { return yy::parser::make_COLON(ctx.loc_); }

[ \t] { ctx.loc_.step(); }
[\n]+  { ctx.loc_.lines(yyleng); ctx.loc_.step(); }

. {
    std::cerr << "Unknown character: " << yytext << std::endl;
}

%%