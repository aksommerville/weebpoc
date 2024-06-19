#ifndef JSC_H
#define JSC_H

struct jsc_node;

struct jsc_context {

  // During compile.
  const char *src;
  int srcc,srcp; // (srcp) is beyond the current token. (tokenp) is the token's start.
  const char *token;
  int tokenc,tokenp,tokentype;
  const char *refname;

  // During encode.
  struct sr_encoder *dst; // WEAK, owned by our caller.

  // AST, generated during compile and read during encode.
  struct jsc_node **nodev;
  int nodec,nodea;

  // Transient import list, generated during compile and flushed at the end of compile.
  char **importv;
  int importc,importa;

  // Finished import list, for checking whether we already have one. Permanent.
  char **doneimportv;
  int doneimportc,doneimporta;
};

void jsc_context_cleanup(struct jsc_context *ctx);

/* (require_closing_tag) is a convenience that truncates (src) on its first "</script>".
 * jsc does not emit HTML, and aside from that one convenience does not consume it.
 */
int jsc_compile(struct jsc_context *ctx,const char *src,int srcc,int require_closing_tag,const char *refname);

/* Emitted text will always end with a newline, and may have inner newlines at tasteful intervals.
 */
int jsc_encode(struct sr_encoder *dst,struct jsc_context *ctx);

/* Log error and return -2.
 * Context for logging in (ctx->tokenp,tokenc).
 */
int jsc_error(struct jsc_context *ctx,const char *fmt,...);

/*--------------------- Private ---------------------------------------*/

#define JSC_TOKENTYPE_STRING 1
#define JSC_TOKENTYPE_NUMBER 2
#define JSC_TOKENTYPE_IDENT 3
#define JSC_TOKENTYPE_LITERAL 4 /* true, false, null, undefined, NaN */
#define JSC_TOKENTYPE_KEYWORD 5
#define JSC_TOKENTYPE_OPERATOR 6

int jsc_importv_add(struct jsc_context *ctx,const char *relpath,int relpathc);

// Caller preps the "During compile" block.
int jsc_compile_inner(struct jsc_context *ctx);

// Drop existing token and queue up the next one. >0 if a new token is present.
int jsc_token_next(struct jsc_context *ctx);

/* 'assert' to examine the current token -- no change of token token state.
 * 'expect' to read the next token, assert it, then read the next.
 */
int jsc_assert(struct jsc_context *ctx,const char *expect,int expectc);
int jsc_assert_tokentype(struct jsc_context *ctx,int tokentype);
int jsc_expect(struct jsc_context *ctx,const char *expect,int expectc);
int jsc_expect_tokentype(struct jsc_context *ctx,int tokentype);

static inline int jsc_isident(char ch) {
  if ((ch>='0')&&(ch<='9')) return 1;
  if ((ch>='a')&&(ch<='z')) return 1;
  if ((ch>='A')&&(ch<='Z')) return 1;
  if (ch=='_') return 1;
  if (ch=='$') return 1;
  return 0;
}

int jsc_is_keyword(const char *src,int srcc);
int jsc_tokentype_for_identifier(const char *src,int srcc);
int jsc_measure_operator(const char *src,int srcc);
const char *jsc_tokentype_repr(int tokentype);

struct jsc_node {
  struct jsc_node *parent; // WEAK
  struct jsc_node **childv; // STRONG
  int childc,childa;
  int type;
  char *text;
  int textc;
  int argv[4];
};

void jsc_node_del(struct jsc_node *node);
struct jsc_node *jsc_node_new();
struct jsc_node *jsc_node_spawn(struct jsc_node *parent,const char *text,int textc);
int jsc_node_set_text(struct jsc_node *node,const char *src,int srcc);
int jsc_node_add_child(struct jsc_node *parent,struct jsc_node *child);
int jsc_node_encode(struct sr_encoder *dst,struct jsc_node *node);

#endif
