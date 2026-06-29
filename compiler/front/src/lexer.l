%{
    import ast;
    #include "parser.tab.hh"
    #define YY_DECL yy::parser::symbol_type yylex()
%}

%option noyywrap

%%

"if" { return yy::parser::make_IF(); }
"else" { return yy::parser::make_ELSE(); }
"while" { return yy::parser::make_WHILE(); }
"func" { return yy::parser::make_FUNC(); }
"struct" { return yy::parser::make_STRUCT(); }


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
        ast::Lit<std::string>(std::move(s))
    );
}

[a-zA-Z_][a-zA-Z0-9_]* {
    return yy::parser::make_VAR(
        ast::Var(std::move(std::string(yytext)))
    );
}

[0-9]+ {
    return yy::parser::make_NUMBER(
        ast::AnyNode(ast::Lit<int>(std::atoi(yytext)))
    );
}

"(" { return yy::parser::make_LPARENTHESIS(); }
")" { return yy::parser::make_RPARENTHESIS(); }
"{" { return yy::parser::make_LBRACE(); }
"}" { return yy::parser::make_RBRACE(); }

">" { return yy::parser::make_A(); }
">=" { return yy::parser::make_AE(); }
"<" { return yy::parser::make_L(); }
"<=" { return yy::parser::make_LE(); }
"==" { return yy::parser::make_E(); }
"!=" { return yy::parser::make_NE(); }

"=" { return yy::parser::make_ASSIGN(); }
"+" { return yy::parser::make_PLUS(); }
"-" { return yy::parser::make_MINUS(); }
"*" { return yy::parser::make_STAR(); }
"/" { return yy::parser::make_SLASH(); }

"," { return yy::parser::make_COMMA(); }
"." { return yy::parser::make_DOT(); }
":" { return yy::parser::make_COLON(); }

[ \t\n] {  }

. {
    std::cerr << "Unknown character: " << yytext << std::endl;
}

%%