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

[a-zA-Z_][a-zA-Z0-9_]* {
    return yy::parser::make_IDENTIFIER(
        ast::AnyNode(ast::Lit<std::string>(std::string(yytext)))
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

[ \t\n] {  }

. {
    std::cerr << "Unknown character: " << yytext << std::endl;
}

%%