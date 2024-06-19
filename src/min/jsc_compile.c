#include "min_internal.h"
#include "jsc.h"

/* Expression.
 */

static int jsc_compile_expression(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "const","let"
 */

static int jsc_compile_declaration(struct jsc_context *ctx,int isconst) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "function"
 */

static int jsc_compile_function(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "if"
 */

static int jsc_compile_if(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "for"
 */

static int jsc_compile_for(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "while"
 */

static int jsc_compile_while(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "do"
 */

static int jsc_compile_do(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "switch"
 */

static int jsc_compile_switch(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "return"
 */

static int jsc_compile_return(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "break"
 */

static int jsc_compile_break(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "continue"
 */

static int jsc_compile_continue(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "throw"
 */

static int jsc_compile_throw(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* "try"
 */

static int jsc_compile_try(struct jsc_context *ctx) {
  return jsc_error(ctx,"TODO %s:%d:%s",__FILE__,__LINE__,__func__);
}

/* Statement.
 */

int jsc_compile_statement(struct jsc_context *ctx) {
  const char *token=ctx->src+ctx->tokenp;
  int tokenc=ctx->tokenc;
  int err;

  if ((tokenc==5)&&!memcmp(token,"const",5)) return jsc_compile_declaration(ctx,1);
  if ((tokenc==3)&&!memcmp(token,"let",3)) return jsc_compile_declaration(ctx,0);
  if ((tokenc==8)&&!memcmp(token,"function",8)) return jsc_compile_function(ctx);
  if ((tokenc==2)&&!memcmp(token,"if",2)) return jsc_compile_if(ctx);
  if ((tokenc==3)&&!memcmp(token,"for",3)) return jsc_compile_for(ctx);
  if ((tokenc==5)&&!memcmp(token,"while",5)) return jsc_compile_while(ctx);
  if ((tokenc==2)&&!memcmp(token,"do",2)) return jsc_compile_do(ctx);
  if ((tokenc==6)&&!memcmp(token,"switch",6)) return jsc_compile_switch(ctx);
  if ((tokenc==6)&&!memcmp(token,"return",6)) return jsc_compile_return(ctx);
  if ((tokenc==5)&&!memcmp(token,"break",5)) return jsc_compile_break(ctx);
  if ((tokenc==8)&&!memcmp(token,"continue",8)) return jsc_compile_continue(ctx);
  if ((tokenc==5)&&!memcmp(token,"throw",5)) return jsc_compile_throw(ctx);
  if ((tokenc==3)&&!memcmp(token,"try",3)) return jsc_compile_try(ctx);

  /* Anything else must be a primary expression followed by a semicolon.
   */
  if ((err=jsc_compile_expression(ctx))<0) return err;
  if ((err=jsc_token_next(ctx))<0) return err;
  if ((err=jsc_assert(ctx,";",1))<0) return err;
  return 0;
}

/* Method in class, including "constructor".
 */

static int jsc_compile_method(struct jsc_context *ctx,int isstatic) {
  return jsc_error(ctx,"TODO %s:%d:%s '%.*s'",__FILE__,__LINE__,__func__,ctx->tokenc,ctx->src+ctx->tokenp);
}

/* Class.
 */

static int jsc_compile_class(struct jsc_context *ctx) {
  int err;
  if ((err=jsc_token_next(ctx))<0) return err;
  if ((err=jsc_assert_tokentype(ctx,JSC_TOKENTYPE_IDENT))<0) return err;
  const char *name=ctx->src+ctx->tokenp;
  int namec=ctx->tokenc;
  //TODO Create and install node.
  if ((err=jsc_expect(ctx,"{",1))<0) return -1;
  for (;;) {

    if (!ctx->tokentype) return jsc_error(ctx,"Unexpected EOF in class body");

    if ((ctx->tokenc==1)&&(ctx->src[ctx->tokenp]=='}')) break;

    int isstatic=0;
    if ((ctx->tokenc==6)&&!memcmp(ctx->src+ctx->tokenp,"static",6)) {
      isstatic=1;
      if ((err=jsc_token_next(ctx))<0) return err;
    }
    if ((err=jsc_assert_tokentype(ctx,JSC_TOKENTYPE_IDENT))<0) return err;
    if ((err=jsc_compile_method(ctx,isstatic))<0) return err;
  }
  return 0;
}

/* Import. Validate, and record the path.
 * Current token is the "import".
 */

static int jsc_compile_import(struct jsc_context *ctx) {
  int err;
  if ((err=jsc_expect(ctx,"{",1))<0) return err;
  for (;;) {
    if ((err=jsc_token_next(ctx))<0) return err;
    if (!err) return jsc_error(ctx,"Unexpected EOF in import");
    if ((ctx->tokenc==1)&&(ctx->src[ctx->tokenp]=='}')) break;
    if ((ctx->tokenc==2)&&!memcmp(ctx->src+ctx->tokenp,"as",2)) return jsc_error(ctx,"Renaming imports not allowed.");
  }
  if ((err=jsc_expect(ctx,"from",4))<0) return err;
  if ((err=jsc_assert_tokentype(ctx,JSC_TOKENTYPE_STRING))<0) return err;
  const char *relpath=ctx->src+ctx->tokenp+1;
  int relpathc=ctx->tokenc-2;
  if ((err=jsc_importv_add(ctx,relpath,relpathc))<0) return err;
  if ((err=jsc_token_next(ctx))<0) return err;
  if ((err=jsc_assert(ctx,";",1))<0) return err;
  return 0;
}

/* Compile in context.
 * Top-level statements.
 */

int jsc_compile_inner(struct jsc_context *ctx) {
  while (ctx->srcp<ctx->srcc) {
    int err=jsc_token_next(ctx);
    if (err<0) return err;
    if (!err) break;
    const char *token=ctx->src+ctx->tokenp;
    int tokenc=ctx->tokenc;

    if ((tokenc==6)&&!memcmp(token,"import",6)) {
      // 'import' gets digested and the path recorded. They'll be resolved after this file is complete.
      if ((err=jsc_compile_import(ctx))<0) return err;

    } else if ((tokenc==6)&&!memcmp(token,"export",6)) {
      // 'export' can be dropped entirely, just keep going from whatever comes next.

    } else if ((tokenc==5)&&!memcmp(token,"class",5)) {
      // 'class' is permitted only at top scope (that's nonstandard, our own quirk).
      if ((err=jsc_compile_class(ctx))<0) return err;

    } else {
      if ((err=jsc_compile_statement(ctx))<0) return err;
    }
  }
  return 0;
}
