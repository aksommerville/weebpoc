#include "min_internal.h"
#include "jsc.h"

/* Next token.
 */

int jsc_token_next(struct jsc_context *ctx) {

  // If the previous token was "(" or "=", allow the next to be a regex literal.
  // Otherwise "/" is an operator.
  int allow_regex=0;
  if ((ctx->tokenc==1)&&(ctx->src[ctx->tokenp]=='(')) allow_regex=1;
  else if ((ctx->tokenc==1)&&(ctx->src[ctx->tokenp]=='=')) allow_regex=1;

  ctx->tokentype=0;
  for (;;) {

    // Termination and whitespace.
    if (ctx->srcp>=ctx->srcc) return 0;
    if ((unsigned char)ctx->src[ctx->srcp]<=0x20) {
      ctx->srcp++;
      continue;
    }

    // Comments.
    int available=ctx->srcc-ctx->srcp;
    if ((available>=2)&&(ctx->src[ctx->srcp]=='/')) {
      if (ctx->src[ctx->srcp+1]=='/') {
        ctx->srcp+=2;
        while ((ctx->srcp<ctx->srcc)&&(ctx->src[ctx->srcp]!=0x0a)) ctx->srcp++;
        continue;
      }
      if (ctx->src[ctx->srcp+1]=='*') {
        ctx->tokenp=ctx->srcp;
        ctx->tokenc=2;
        ctx->srcp+=2;
        for (;;) {
          if (ctx->srcp>ctx->srcc-2) return jsc_error(ctx,"Unclosed block comment");
          if ((ctx->src[ctx->srcp]=='*')&&(ctx->src[ctx->srcp+1]=='/')) {
            ctx->srcp+=2;
            break;
          }
          ctx->srcp++;
        }
        continue;
      }
    }

    // String and regex literals.
    if (
      (ctx->src[ctx->srcp]=='"')||(ctx->src[ctx->srcp]=='\'')||(ctx->src[ctx->srcp]=='`')||
      (allow_regex&&(ctx->src[ctx->srcp]=='/'))
    ) {
      char quote=ctx->src[ctx->srcp];
      ctx->tokenp=ctx->srcp;
      ctx->tokenc=1;
      ctx->srcp++;
      for (;;) {
        if (ctx->srcp>=ctx->srcc) return jsc_error(ctx,"Unclosed string literal");
        if (ctx->src[ctx->srcp]==quote) {
          ctx->srcp++;
          break;
        }
        if (ctx->src[ctx->srcp]=='\\') ctx->srcp+=2;
        else ctx->srcp+=1;
      }
      ctx->tokenc=ctx->srcp-ctx->tokenp;
      ctx->tokentype=JSC_TOKENTYPE_STRING;
      return 1;
    }

    // Number literals.
    if ((ctx->src[ctx->srcp]>='0')&&(ctx->src[ctx->srcp]<='9')) {
      ctx->tokenp=ctx->srcp;
      ctx->tokenc=0;
      ctx->tokentype=JSC_TOKENTYPE_NUMBER;
      while (ctx->srcp<ctx->srcc) {
        char ch=ctx->src[ctx->srcp];
             if ((ch>='0')&&(ch<='9')) ;
        else if ((ch>='a')&&(ch<='z')) ;
        else if ((ch>='A')&&(ch<='Z')) ;
        else if (ch=='.') ;
        else break;
        ctx->srcp++;
        ctx->tokenc++;
      }
      return 1;
    }

    // Identifiers and keywords.
    if (jsc_isident(ctx->src[ctx->srcp])) {
      ctx->tokenp=ctx->srcp;
      ctx->tokenc=1;
      ctx->srcp++;
      while ((ctx->srcp<ctx->srcc)&&jsc_isident(ctx->src[ctx->srcp])) {
        ctx->srcp++;
        ctx->tokenc++;
      }
      ctx->tokentype=jsc_tokentype_for_identifier(ctx->src+ctx->tokenp,ctx->tokenc);
      return 1;
    }

    // Everything else is an operator.
    ctx->tokenp=ctx->srcp;
    if ((ctx->tokenc=jsc_measure_operator(ctx->src+ctx->srcp,ctx->srcc-ctx->srcp))<1) {
      return jsc_error(ctx,"Unexpected character '%c'",ctx->src[ctx->srcp]);
    }
    ctx->tokentype=JSC_TOKENTYPE_OPERATOR;
    ctx->srcp+=ctx->tokenc;
    return 1;
  }
}

/* Test keywords.
 */

// Includes reserved words and other conditionally reserved things, the broadest set of things that shouldn't be used as identifiers.
// Also, oddly "constructor" and "window" were not in the list, but ought to be.
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Lexical_grammar
static const char *jsc_keywords[]={
  "abstract",
  "arguments",
  "as",
  "async",
  "await",
  "boolean",
  "break",
  "byte",
  "case",
  "catch",
  "char",
  "class",
  "const",
  "constructor",
  "continue",
  "debugger",
  "default",
  "delete",
  "do",
  "double",
  "else",
  "enum",
  "eval",
  "export",
  "extends",
  "false",
  "final",
  "finally",
  "float",
  "for",
  "from",
  "function",
  "get",
  "goto",
  "if",
  "implements",
  "import",
  "in",
  "instanceof",
  "int",
  "interface",
  "let",
  "long",
  "native",
  "new",
  "null",
  "of",
  "package",
  "private",
  "protected",
  "public",
  "return",
  "set",
  "short",
  "static",
  "super",
  "switch",
  "synchronized",
  "this",
  "throw",
  "throws",
  "transient",
  "true",
  "try",
  "typeof",
  "var",
  "void",
  "volatile",
  "while",
  "window",
  "with",
  "yield",
};

int jsc_is_keyword(const char *src,int srcc) {
  if (!src||(srcc<1)) return 0;
  int lo=0,hi=sizeof(jsc_keywords)/sizeof(void*);
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int cmp=memcmp(src,jsc_keywords[ck],srcc);
    if (!cmp) {
      if (jsc_keywords[ck][srcc]) cmp=-1;
      else return 1;
    }
    if (cmp<0) hi=ck;
    else lo=ck+1;
  }
  return 0;
}

/* Token type for identifier-like tokens.
 */
 
int jsc_tokentype_for_identifier(const char *src,int srcc) {
  if (!src||(srcc<1)) return 0;
  if ((srcc==4)&&!memcmp(src,"true",4)) return JSC_TOKENTYPE_LITERAL;
  if ((srcc==5)&&!memcmp(src,"false",5)) return JSC_TOKENTYPE_LITERAL;
  if ((srcc==4)&&!memcmp(src,"null",4)) return JSC_TOKENTYPE_LITERAL;
  if ((srcc==9)&&!memcmp(src,"undefined",9)) return JSC_TOKENTYPE_LITERAL;
  if ((srcc==3)&&!memcmp(src,"NaN",3)) return JSC_TOKENTYPE_LITERAL;
  if (jsc_is_keyword(src,srcc)) return JSC_TOKENTYPE_KEYWORD;
  return JSC_TOKENTYPE_IDENT;
}

/* Multibyte operators.
 * Single bytes, just assume it's kosher.
 */

int jsc_measure_operator(const char *src,int srcc) {
  if (!src||(srcc<1)) return 0;

  if (srcc>=4) {
    if (!memcmp(src,">>>=",4)) return 4;
  }

  if (srcc>=3) {
    if (!memcmp(src,"===",3)) return 3;
    if (!memcmp(src,"!==",3)) return 3;
    if (!memcmp(src,">>>",3)) return 3;
    if (!memcmp(src,"&&=",3)) return 3;
    if (!memcmp(src,"||=",3)) return 3;
    if (!memcmp(src,">>=",3)) return 3;
    if (!memcmp(src,"<<=",3)) return 3;
  }

  if (srcc>=2) {
    if (src[1]=='=') switch (src[0]) {
      case '=':
      case '!':
      case '<':
      case '>':
      case '+':
      case '-':
      case '*':
      case '/':
      case '&':
      case '|':
      case '^':
        return 2;
    }
    if (src[0]==src[1]) switch (src[0]) {
      case '<':
      case '>':
      case '?':
      case '&':
      case '|':
      case '*':
        return 2;
    }
    if (!memcmp(src,"=>",2)) return 2;
  }

  return 1;
}

/* Tokentype.
 */
 
const char *jsc_tokentype_repr(int tokentype) {
  switch (tokentype) {
    #define _(tag) case JSC_TOKENTYPE_##tag: return #tag;
    _(STRING)
    _(NUMBER)
    _(IDENT)
    _(LITERAL)
    _(KEYWORD)
    _(OPERATOR)
    #undef _
  }
  return "(unknown)";
}
