#include <stdio.h>
#include "../programTree/tree.h"
#include "graphDump.h"

const char* getColor(types type)
{
    switch (type)
    {
    case ND_POADD:
    case ND_ADD:
    case ND_SUB:
    case ND_DIV:
    case ND_MUL:
    case ND_POW:
    case ND_SIN:
    case ND_COS:
    case ND_LOG:
    case ND_SQRT:
        return "grey";
        

    case ND_NUM:
    case ND_VAR:
        return "blue";
        

    case ND_RCIB:
    case ND_LCIB:
    case ND_LCUB:
    case ND_RCUB:
        return "yellow";
        

    case ND_EQ:
    case ND_PR:
    case ND_GET:
    case ND_IF:
    case ND_FOR:
    case ND_FUN:
    case ND_FUNCALL:
    case ND_RET:
    case ND_ENDFOR:
        return "green";
        
        
    case ND_ISEQ:
    case ND_NISEQ:
    case ND_AB:
    case ND_LS:
    case ND_ABE:
    case ND_LSE:
        return "orange";
        
    case ND_EOT:
    case ND_SEP:
        return "red";
        
    case ND_GETDIFF:
        return "purple";
        
    default:
        return "blue";
        
    }
}

const char* convertTypeToStr(types type)
{
    switch (type)
    {
    case ND_ADD:
        return "+";
        
    case ND_SUB:
        return "-";
        
    case ND_MUL:
        return "*";
        
    case ND_DIV:
        return "/";
         
    case ND_NUM:
        return "num";
        
    case ND_POW:
        return "^";
        
    case ND_SIN:
        return "sin";
        
    case ND_COS:
        return "cos";
        
    case ND_LOG:
        return "log";
        
    case ND_VAR:
        return "var";
        
    case ND_POADD:
        return "++";
        
    case ND_LCIB:
        return "(";
        
    case ND_ISEQ:
        return "==";
        
    case ND_NISEQ:
        return "!=";
        
    case ND_LS:
        return "belove";
        
    case ND_AB:
        return "above";
        
    case ND_ABE:
        return "aboquel";
        
    case ND_LSE:
        return "beloquel";
        
    case ND_RCIB:
        return ")";
        
    case ND_LCUB:
        return "fig braket";
        
    case ND_RCUB:
        return "fig braket";
        
    case ND_FOR:
        return "for";
        
    case ND_IF:
        return "if";
        
    case ND_FUN:
        return "function";
        
    case ND_EQ:
        return "=";

    case ND_SEP:
        return "newLine";

    case ND_EOT:
        return "EOT";

    case ND_ENDFOR:
        return "END_FOR";

    case ND_PR:
        return "print";

    case ND_RET:
        return "return";

    case ND_FUNCALL:
        return "funcall";
    
    case ND_SQRT:
        return "sqrt";
    
    case ND_GET:
        return "get";

    case ND_GETDIFF:
        return "diff";

    default:
        return "error";
    }
}
