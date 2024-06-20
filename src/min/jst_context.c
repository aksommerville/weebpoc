#include "min_internal.h"
#include "jst.h"
#include <stdarg.h>

static int jst_minify_internal(struct jst_context *ctx);

/* Cleanup.
 */
 
static void jst_ident_cleanup(struct jst_ident *ident) {
  if (ident->src) free(ident->src);
  if (ident->dst) free(ident->dst);
}

void jst_context_cleanup(struct jst_context *ctx) {
  if (ctx->importedv) {
    while (ctx->importedc-->0) free(ctx->importedv[ctx->importedc]);
    free(ctx->importedv);
  }
  while (ctx->identc>0) {
    ctx->identc--;
    jst_ident_cleanup(ctx->identv+ctx->identc);
  }
  sr_encoder_cleanup(&ctx->scratch);
  memset(ctx,0,sizeof(struct jst_context));
}

/* Imports list.
 */
 
static int jst_importedv_has(const struct jst_context *ctx,const char *path) {
  int i=ctx->importedc;
  while (i-->0) {
    if (!strcmp(ctx->importedv[i],path)) return 1;
  }
  return 0;
}

static int jst_importedv_add(struct jst_context *ctx,const char *path,int pathc) {
  if (!path||(pathc<1)) return -1;
  if (ctx->importedc>=ctx->importeda) {
    int na=ctx->importeda+32;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(ctx->importedv,sizeof(void*)*na);
    if (!nv) return -1;
    ctx->importedv=nv;
    ctx->importeda=na;
  }
  char *nv=malloc(pathc+1);
  if (!nv) return -1;
  memcpy(nv,path,pathc);
  nv[pathc]=0;
  ctx->importedv[ctx->importedc++]=nv;
  return 0;
}

/* Identifiers list.
 */
 
static struct jst_ident *jst_ident_search(const struct jst_context *ctx,const char *src,int srcc) {
  struct jst_ident *ident=ctx->identv;
  int i=ctx->identc;
  for (;i-->0;ident++) {
    if (srcc!=ident->srcc) continue;
    if (memcmp(src,ident->src,srcc)) continue;
    return ident;
  }
  return 0;
}

static struct jst_ident *jst_ident_new(struct jst_context *ctx,const char *src,int srcc) {
  if (ctx->identc>=ctx->identa) {
    int na=ctx->identa+128;
    if (na>INT_MAX/sizeof(struct jst_ident)) return 0;
    void *nv=realloc(ctx->identv,sizeof(struct jst_ident)*na);
    if (!nv) return 0;
    ctx->identv=nv;
    ctx->identa=na;
  }
  char tmp[8];
  int tmpc=identifier_from_sequence(tmp,sizeof(tmp),ctx->identc);
  if ((tmpc<1)||(tmpc>sizeof(tmp))) return 0;
  char *nsrc=malloc(srcc+1);
  if (!nsrc) return 0;
  memcpy(nsrc,src,srcc);
  nsrc[srcc]=0;
  char *ndst=malloc(tmpc+1);
  if (!ndst) { free(nsrc); return 0; }
  memcpy(ndst,tmp,tmpc);
  ndst[tmpc]=0;
  struct jst_ident *ident=ctx->identv+ctx->identc++;
  memset(ident,0,sizeof(struct jst_ident));
  ident->src=nsrc;
  ident->srcc=srcc;
  ident->dst=ndst;
  ident->dstc=tmpc;
  return ident;
}

/* Generate the identifiers-declaration line and insert at the top of output.
 */
 
static int jst_declare_identifiers(struct jst_context *ctx) {
  
  // Before jumping in, eliminate any unused identifiers.
  int i=ctx->identc;
  while (i-->0) {
    struct jst_ident *ident=ctx->identv+i;
    if (ident->refc) continue;
    jst_ident_cleanup(ident);
    ctx->identc--;
    memmove(ident,ident+1,sizeof(struct jst_ident)*(ctx->identc-i));
  }
  
  if (ctx->identc<1) return 0;
  ctx->scratch.c=0;
  if (sr_encode_raw(&ctx->scratch,"const [",-1)<0) return -1;
  const struct jst_ident *ident=ctx->identv;
  for (i=ctx->identc;i-->0;ident++) {
    if (sr_encode_raw(&ctx->scratch,ident->dst,ident->dstc)<0) return -1;
    if (i) {
      if (sr_encode_u8(&ctx->scratch,',')<0) return -1;
    }
  }
  if (sr_encode_raw(&ctx->scratch,"]=\"",-1)<0) return -1;
  for (ident=ctx->identv,i=ctx->identc;i-->0;ident++) {
    if (sr_encode_raw(&ctx->scratch,ident->src,ident->srcc)<0) return -1;
    if (i) {
      if (sr_encode_u8(&ctx->scratch,',')<0) return -1;
    }
  }
  if (sr_encode_raw(&ctx->scratch,"\".split(',');\n",-1)<0) return -1;
  if (sr_encoder_replace(ctx->dst,ctx->startp,0,ctx->scratch.v,ctx->scratch.c)<0) return -1;
  return 0;
}

/* Receive one real token.
 * If it's an identifier and we determine it can be reduced, emit the reduced identifier.
 * Otherwise emit verbatim.
 */
 
static int jst_minify_token(struct jst_context *ctx,const char *token,int tokenc) {
  if (!token||(tokenc<1)) return -1;
  struct jst_ident *ident=0;
  
  //XXX This is all kinds of broken. In particular, "direct" identifiers would need their scope tracked, to remove later.
  // I'm going to back up and simplify a little: JS minification will only be for inlining imports and removing whitespace.
  goto _verbatim_;
  //...ha ha ha, it actually got smaller when i stubbed this. :P
  
  /* Not an identifier, emit verbatim.
   */
  if (!jst_isident(token[0])) goto _verbatim_;
  if ((token[0]>='0')&&(token[0]<='9')) goto _verbatim_;
  if (jst_is_keyword(token,tokenc)) goto _verbatim_;
  
  /* Intern the identifier.
   */
  if (!(ident=jst_ident_search(ctx,token,tokenc))) {
    if (!(ident=jst_ident_new(ctx,token,tokenc))) return -1;
    if ((ctx->pvtokenc==5)&&!memcmp(ctx->pvtoken,"const",5)) ident->direct=1;
    else if ((ctx->pvtokenc==3)&&!memcmp(ctx->pvtoken,"let",3)) ident->direct=1;
    else if ((ctx->pvtokenc==5)&&!memcmp(ctx->pvtoken,"class",5)) ident->direct=1;
    else if ((ctx->pvtokenc==8)&&!memcmp(ctx->pvtoken,"function",8)) ident->direct=1;
  }
  
  /* If it's "direct", emit the alias instead.
   * This does not count toward (refc). There's no need to preserve the original token.
   * But we still need space insertion, so hijack the "_verbatim_" case.
   */
  if (ident->direct) {
    token=ident->dst;
    tokenc=ident->dstc;
    goto _verbatim_;
  }
  
  /* If it's a known token, and was preceded by dot, remove the dot and replace with "[DST]".
   * Increment (refc) so we know to preserve this alias.
   */
  if (ident&&(ctx->dst->c>=1)&&(((char*)ctx->dst->v)[ctx->dst->c-1]=='.')) {
    if ((ctx->dst->c>=2)&&(((char*)ctx->dst->v)[ctx->dst->c-2]=='?')) {
      // ...unless it's "a?.b", in which case we want "a?.[B]", don't remove the dot.
    } else {
      ctx->dst->c--;
    }
    if (sr_encode_u8(ctx->dst,'[')<0) return -1;
    if (sr_encode_raw(ctx->dst,ident->dst,ident->dstc)<0) return -1;
    if (sr_encode_u8(ctx->dst,']')<0) return -1;
    ident->refc++;
    return 0;
  }
  
  /* The general case, just emit the identifier.
   */
 _verbatim_:;
  if (ctx->dst->c&&jst_isident(((char*)ctx->dst->v)[ctx->dst->c-1])&&jst_isident(token[0])) {
    if (sr_encode_u8(ctx->dst,' ')<0) return -1;
  }
  if (sr_encode_raw(ctx->dst,token,tokenc)<0) return -1;
  return 0;
}

/* Measure and emit one string interpolation unit.
 * Input should start right *after* the "${", and we do not consume the terminating "}".
 */
 
static int jst_minify_interpolable_unit(struct jst_context *ctx,const char *src,int srcc) {
  int srcp=0,depth=0,err;
  for (;;) {
    if ((err=jst_measure_space(ctx,src+srcp,srcc-srcp))<0) return err;
    srcp+=err;
    if (srcp>=srcc) return -1;
    if (src[srcp]=='}') {
      if (!depth--) return srcp;
      if (sr_encode_u8(ctx->dst,'}')<0) return -1;
      srcp++;
    } else if (src[srcp]=='{') {
      depth++;
      if (sr_encode_u8(ctx->dst,'{')<0) return -1;
      srcp++;
    } else {
      const char *token=src+srcp;
      int tokenc=jst_measure_token(ctx,token,srcc-srcp);
      if (tokenc<1) return jst_error(ctx,token,"Failed to measure token.");
      if ((err=jst_minify_token(ctx,token,tokenc))<0) return err;
      srcp+=tokenc;
    }
  }
}

/* Emit a grave string with possible interpolation.
 */
 
static int jst_minify_interpolable_string(struct jst_context *ctx,const char *src,int srcc) {
  if (!src||(srcc<2)||(src[0]!='`')||(src[srcc-1]!='`')) return -1;
  src++;
  srcc-=2;
  int srcp=0,err;
  if (sr_encode_u8(ctx->dst,'`')<0) return -1;
  while (srcp<srcc) {
    if ((srcp<=srcc-2)&&(src[srcp]=='$')&&(src[srcp+1]=='}')) {
      srcp+=2;
      if ((err=jst_minify_interpolable_unit(ctx,src+srcp,srcc-srcp))<0) return err;
      if ((srcp>=srcc)||(src[srcp]!='}')) return jst_error(ctx,src+srcp,"Expected '}' to close string interpolation unit.");
      srcp++;
    } else {
      if (sr_encode_u8(ctx->dst,src[srcp])<0) return -1;
      srcp++;
    }
  }
  if (sr_encode_u8(ctx->dst,'`')<0) return -1;
  return 0;
}

/* Swap out what's needed to enter alternate files.
 * Provide a blank jst_context at push, and the same at pop. It won't need cleaned up.
 */
 
static void jst_push(struct jst_context *tmp,struct jst_context *ctx,const char *src,int srcc,const char *refname) {
  tmp->src=ctx->src;
  tmp->srcc=ctx->srcc;
  tmp->refname=ctx->refname;
  ctx->src=src;
  ctx->srcc=srcc;
  ctx->refname=refname;
  ctx->pvtoken=0;
  ctx->pvtokenc=0;
}

static void jst_pop(struct jst_context *ctx,const struct jst_context *tmp) {
  ctx->src=tmp->src;
  ctx->srcc=tmp->srcc;
  ctx->refname=tmp->refname;
  ctx->pvtoken=0;
  ctx->pvtokenc=0;
}

/* Parse an import statement, resolve path, read file, reenter minification.
 * Input begin immediately after the 'import' token.
 * Returns consumed length of source.
 */
 
static int jst_import(struct jst_context *ctx,const char *src,int srcc) {

  /* First, validate the syntax, measure thru semicolon, and extract the relative path.
   */
  int srcp=0,err;
  const char *relpath=0;
  int relpathc=0;
  const char *expectv[]={"{",".","from","'",";"};
  int expectc=sizeof(expectv)/sizeof(void*);
  #define EXPECTNEXT { \
    if (!expectc) return -1; \
    expectc--; \
    memmove(expectv,expectv+1,sizeof(void*)*expectc); \
  }
  for (;;) {
    if ((err=jst_measure_space(ctx,src+srcp,srcc-srcp))<0) return jst_error(ctx,src+srcp,"Unspecified lexical error.");
    srcp+=err;
    if (srcp>=srcc) break;
    const char *token=src+srcp;
    int tokenc=jst_measure_token(ctx,token,srcc-srcp);
    if (tokenc<1) return jst_error(ctx,token,"Failed to measure next token.");
    srcp+=tokenc;
    
    const char *expect=expectv[0];
    if (expect[0]=='.') { // Inside curlies. No need to validate hard, just fail on "as", and advance on "}".
      if ((tokenc==2)&&!memcmp(token,"as",2)) return jst_error(ctx,token,"Renaming imports not permitted.");
      if ((tokenc==1)&&(token[0]=='}')) {
        EXPECTNEXT
        continue;
      }
      // Anything else should be a comma or identifier, and we ignore it.
      continue;
    }
    if (expect[0]=='\'') { // Require import path, here's the important part.
      if ((tokenc<2)||((token[0]!='"')&&(token[0]!='\''))) return jst_error(ctx,token,"Expected quoted string for import, before '%.*s'.",tokenc,token);
      relpath=token+1;
      relpathc=tokenc-2;
      EXPECTNEXT
      continue;
    }
    if (expect[0]==';') {
      if ((tokenc!=1)||(token[0]!=';')) return jst_error(ctx,token,"Expected ';' before '%.*s'.",tokenc,token);
      EXPECTNEXT
      break;
    }
    if (memcmp(expect,token,tokenc)||expect[tokenc]) {
      return jst_error(ctx,token,"Expected '%s' before '%.*s'.",expect,tokenc,token);
    }
    EXPECTNEXT
  }
  #undef EXPECTNEXT
  if (expectc) return jst_error(ctx,src,"Malformed import.");
  if (!relpath||!relpathc) return jst_error(ctx,src,"Malformed import.");
  
  /* Determine full path and check list of prior imports.
   */
  char path[1024];
  int pathc=resolve_path(path,sizeof(path),ctx->refname,relpath,relpathc);
  if ((pathc<1)||(pathc>=sizeof(path))) return jst_error(ctx,relpath,"Failed to resolve import path.");
  if (jst_importedv_has(ctx,path)) return srcp;
  if (jst_importedv_add(ctx,path,pathc)<0) return -1;
  
  /* Reenter for the imported file.
   */
  char *subsrc=0;
  int subsrcc=file_read(&subsrc,path);
  if (subsrcc<0) return jst_error(ctx,relpath,"%s: Failed to read file.",path);
  struct jst_context tmp={0};
  jst_push(&tmp,ctx,subsrc,subsrcc,path);
  err=jst_minify_internal(ctx);
  jst_pop(ctx,&tmp);
  if (err<0) return err;
  free(subsrc);
  return srcp;
}

/* Minify, top level.
 */

int jst_minify(struct sr_encoder *dst,struct jst_context *ctx,const char *src,int srcc,const char *refname) {
  int err;
  if (!dst||!ctx) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!refname) refname="<unknown>";
  ctx->startp=dst->c;
  ctx->dst=dst;
  ctx->src=src;
  ctx->srcc=srcc;
  ctx->refname=refname;
  ctx->pvtokenc=0;
  if ((err=jst_minify_internal(ctx))<0) return err;
  if ((err=jst_declare_identifiers(ctx))<0) return err;
  return 0;
}

static int jst_minify_internal(struct jst_context *ctx) {
  const char *src=ctx->src;
  int srcc=ctx->srcc;
  int srcp=0,err;
  for (;;) {
    if ((err=jst_measure_space(ctx,src+srcp,srcc-srcp))<0) return jst_error(ctx,src+srcp,"Unspecified lexical error.");
    srcp+=err;
    if (srcp>=srcc) break;
    const char *token=src+srcp;
    int tokenc=jst_measure_token(ctx,token,srcc-srcp);
    if (tokenc<1) return jst_error(ctx,token,"Failed to measure next token.");
    srcp+=tokenc;
    
    if (ctx->ignore) continue;
    
    if ((tokenc==6)&&!memcmp(token,"import",6)) {
      // At "import", inline the referenced file recursively.
      if ((err=jst_import(ctx,src+srcp,srcc-srcp))<0) return jst_error(ctx,token,"Unspecified error resolving import.");
      srcp+=err;
      
    } else if ((tokenc==6)&&!memcmp(token,"export",6)) {
      // "export" is meaningless to us, since we're concatenating all imports.
      
    } else if ((tokenc>=2)&&(token[0]=='`')) {
      // Interpolable strings, we need to enter each "${...}" unit.
      if ((err=jst_minify_interpolable_string(ctx,token,tokenc))<0) return jst_error(ctx,token,"Unspecified error inside string.");
      
    } else {
      // Everything else, check for 1:1 token reduction or emit verbatim.
      if ((err=jst_minify_token(ctx,token,tokenc))<0) return jst_error(ctx,token,"Unspecified error.");
      
    }
    ctx->pvtoken=token;
    ctx->pvtokenc=tokenc;
  }
  // Output a newline at the end of each file. It's not needed, but it's cheap I feel goes a long way for traceability.
  if (sr_encode_u8(ctx->dst,0x0a)<0) return -1;
  // IGNORE blocks implicitly close at end of file.
  ctx->ignore=0;
  return 0;
}

/* Log error.
 */
 
int jst_error(struct jst_context *ctx,const char *src,const char *fmt,...) {
  if (ctx->logged_error) return -2;
  ctx->logged_error=1;
  if ((src>=ctx->src)&&(src<=ctx->src+ctx->srcc)) {
    fprintf(stderr,"%s:%d: ",ctx->refname,lineno(ctx->src,src-ctx->src));
  } else {
    fprintf(stderr,"%s: ",ctx->refname);
  }
  va_list vargs;
  va_start(vargs,fmt);
  vfprintf(stderr,fmt,vargs);
  fprintf(stderr,"\n");
  return -2;
}
