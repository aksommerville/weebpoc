#include "min_internal.h"
#include "jsc.h"
#include <stdarg.h>

/* Cleanup.
 */
 
void jsc_context_cleanup(struct jsc_context *ctx) {
  if (ctx->nodev) {
    while (ctx->nodec-->0) {
      jsc_node_del(ctx->nodev[ctx->nodec]);
    }
    free(ctx->nodev);
  }
  if (ctx->importv) {
    while (ctx->importc-->0) free(ctx->importv[ctx->importc]);
    free(ctx->importv);
  }
  if (ctx->doneimportv) {
    while (ctx->doneimportc-->0) free(ctx->doneimportv[ctx->doneimportc]);
    free(ctx->doneimportv);
  }
  memset(ctx,0,sizeof(struct jsc_context));
}

/* Compile one import.
 */

static int jsc_compile_import(struct jsc_context *ctx,const char *path) {
  int i=ctx->doneimportc;
  while (i-->0) if (!strcmp(ctx->doneimportv[i],path)) return 0;
  if (ctx->doneimportc>=ctx->doneimporta) {
    int na=ctx->doneimporta+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(ctx->doneimportv,sizeof(void*)*na);
    if (!nv) return -1;
    ctx->doneimportv=nv;
    ctx->doneimporta=na;
  }
  char *nv=strdup(path);
  if (!nv) return -1;
  ctx->doneimportv[ctx->doneimportc++]=nv;
  char *src=0;
  int srcc=file_read(&src,path);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file\n",path);
    return -2;
  }
  int err=jsc_compile(ctx,src,srcc,0,path);
  free(src);
  return err;
}

/* Build AST from text.
 */

int jsc_compile(struct jsc_context *ctx,const char *src,int srcc,int require_closing_tag,const char *refname) {

  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int result=srcc;
  if (require_closing_tag) {
    int stopp=srcc-9;
    int i=0; for (;i<stopp;i++) {
      if (!memcmp(src+i,"</script>",9)) {
        srcc=i;
        result=i+9;
        break;
      }
    }
  }

  ctx->src=src;
  ctx->srcc=srcc;
  ctx->refname=refname;
  ctx->srcp=0;
  ctx->tokenc=0;
  ctx->tokenp=0;
  ctx->tokentype=0;

  int err=jsc_compile_inner(ctx);
  if (err<0) return err;
  
  if (ctx->importv) {
    char **importv=ctx->importv;
    int importc=ctx->importc;
    ctx->importv=0;
    ctx->importc=0;
    ctx->importa=0;
    while (importc-->0) {
      if (err>=0) err=jsc_compile_import(ctx,importv[importc]);
      free(importv[importc]);
    }
    free(importv);
    if (err<0) return err;
  }

  return result;
}

/* Generate output text.
 */

int jsc_encode(struct sr_encoder *dst,struct jsc_context *ctx) {
  int i=ctx->nodec,err;
  struct jsc_node **v=ctx->nodev;
  for (;i-->0;v++) {
    if ((err=jsc_node_encode(dst,*v))<0) return err;
  }
  return 0;
}

/* Log error.
 */

int jsc_error(struct jsc_context *ctx,const char *fmt,...) {
  fprintf(stderr,"%s:%d: ",ctx->refname,lineno(ctx->src,ctx->tokenp));
  va_list vargs;
  va_start(vargs,fmt);
  vfprintf(stderr,fmt,vargs);
  fprintf(stderr,"\n");
  return -2;
}

/* Token assertions.
 */

int jsc_assert(struct jsc_context *ctx,const char *expect,int expectc) {
  if (!expect) expectc=0; else if (expectc<0) { expectc=0; while (expect[expectc]) expectc++; }
  if ((expectc!=ctx->tokenc)||memcmp(expect,ctx->src+ctx->tokenp,expectc)) {
    return jsc_error(ctx,"Expected '%.*s' before '%.*s'",expectc,expect,ctx->tokenc,ctx->src+ctx->tokenp);
  }
  return 0;
}

int jsc_assert_tokentype(struct jsc_context *ctx,int tokentype) {
  if (ctx->tokentype!=tokentype) {
    return jsc_error(ctx,"Expected %s before '%.*s'",jsc_tokentype_repr(tokentype),ctx->tokenc,ctx->src+ctx->tokenp);
  }
  return 0;
}

int jsc_expect(struct jsc_context *ctx,const char *expect,int expectc) {
  int err;
  if ((err=jsc_token_next(ctx))<0) return err;
  if ((err=jsc_assert(ctx,expect,expectc))<0) return err;
  if ((err=jsc_token_next(ctx))<0) return err;
  return 0;
}

int jsc_expect_tokentype(struct jsc_context *ctx,int tokentype) {
  int err;
  if ((err=jsc_token_next(ctx))<0) return err;
  if ((err=jsc_assert_tokentype(ctx,tokentype))<0) return err;
  if ((err=jsc_token_next(ctx))<0) return err;
  return 0;
}

/* Add import.
 */
 
int jsc_importv_add(struct jsc_context *ctx,const char *relpath,int relpathc) {
  if (!relpath||(relpathc<1)) return -1;
  const char *pfx=ctx->refname;
  if (!pfx) pfx="";
  int pfxc=0;
  while (pfx[pfxc]) pfxc++;
  while (pfxc&&(pfx[pfxc-1]=='/')) pfxc--;
  while (pfxc&&(pfx[pfxc-1]!='/')) pfxc--;
  while (pfxc&&(pfx[pfxc-1]=='/')) pfxc--;
  for (;;) {
    if ((relpathc>=3)&&!memcmp(relpath,"../",3)) {
      while (pfxc&&(pfx[pfxc-1]!='/')) pfxc--;
      while (pfxc&&(pfx[pfxc-1]=='/')) pfxc--;
      relpath+=3;
      relpathc-=3;
    } else if ((relpathc>=2)&&!memcmp(relpath,"./",2)) {
      relpath+=2;
      relpathc-=2;
    } else if ((relpathc>=1)&&(relpath[0]=='/')) {
      relpath++;
      relpathc--;
    } else {
      break;
    }
  }
  if (ctx->importc>=ctx->importa) {
    int na=ctx->importa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(ctx->importv,sizeof(void*)*na);
    if (!nv) return -1;
    ctx->importv=nv;
    ctx->importa=na;
  }
  int pathc=pfxc+1+relpathc;
  char *path=malloc(pathc+1);
  if (!path) return -1;
  memcpy(path,pfx,pfxc);
  path[pfxc]='/';
  memcpy(path+pfxc+1,relpath,relpathc);
  path[pathc]=0;
  ctx->importv[ctx->importc++]=path;
  return 0;
}
