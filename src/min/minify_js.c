#include "min_internal.h"
#include "jsc.h"
#include <stdarg.h>

/* Log error.
 */

static int js_error(const char *src,int srcp,const char *refname,const char *fmt,...) {
  if (!refname) {
    int pfxp=(int)(src-min.src);
    if ((pfxp<0)||(pfxp>min.srcc-srcp)) {
      refname="<unknown>";
    } else {
      refname=min.srcpath;
      src=min.src;
      srcp+=pfxp;
    }
  }
  fprintf(stderr,"%s:%d: ",refname,lineno(src,srcp));
  va_list vargs;
  va_start(vargs,fmt);
  vfprintf(stderr,fmt,vargs);
  fprintf(stderr,"\n");
  return -2;
}

#if 0 /*XXX Replace with something more flexible, with a full AST. */
static int minify_js_1(const char *src,int srcc,int require_closing_tag,const char *refname);

/* List of all imported files.
 */

static void imports_clear() {
  while (min.importedc>0) {
    min.importedc--;
    free(min.importedv[min.importedc]);
  }
}

// Adds to min.importedv if needed. Returns >0 if was already listed, or 0 if newly added.
static int imports_check(const char *path,int pathc) {
  if (!path) return 0;
  if (pathc<0) { pathc=0; while (path[pathc]) pathc++; }
  char **q=min.importedv;
  int i=min.importedc;
  for (;i-->0;q++) {
    if (memcmp(*q,path,pathc)) continue;
    if ((*q)[pathc]) continue;
    return 1;
  }
  if (min.importedc>=min.importeda) {
    int na=min.importeda+16;
    void *nv=realloc(min.importedv,sizeof(void*)*na);
    if (!nv) return -1;
    min.importedv=nv;
    min.importeda=na;
  }
  char *nv=malloc(pathc+1);
  if (!nv) return -1;
  memcpy(nv,path,pathc);
  nv[pathc]=0;
  min.importedv[min.importedc++]=nv;
  return 0;
}

/* List of identifiers.
 */

static void ident_cleanup(struct ident *ident) {
  if (ident->original) free(ident->original);
}

static void idents_clear() {
  while (min.identc>0) {
    min.identc--;
    ident_cleanup(min.identv+min.identc);
  }
}

static struct ident *ident_intern(const char *name,int namec,int *fresh) {
  if (fresh) *fresh=0;
  struct ident *ident=min.identv;
  int i=min.identc;
  for (;i-->0;ident++) {
    if (ident->originalc!=namec) continue;
    if (memcmp(ident->original,name,namec)) continue;
    return ident;
  }
  if (min.identc>=min.identa) {
    int na=min.identa+128;
    if (na>INT_MAX/sizeof(struct ident)) return 0;
    void *nv=realloc(min.identv,sizeof(struct ident)*na);
    if (!nv) return 0;
    min.identv=nv;
    min.identa=na;
  }
  char *nv=malloc(namec+1);
  if (!nv) return 0;
  memcpy(nv,name,namec);
  nv[namec]=0;
  if (fresh) *fresh=1;
  ident=min.identv+min.identc++;
  memset(ident,0,sizeof(struct ident));
  ident->original=nv;
  ident->originalc=namec;
  ident->seq=min.identc-1;
  ident->preserve=1; // True unless our caller can establish otherwise. (preserve) is always safe, but using it unnecessarily reduces compression.
  return ident;
}

/* Token tests.
 */

static int js_isident(char src) {
  if ((src>='a')&&(src<='z')) return 1;
  if ((src>='A')&&(src<='Z')) return 1;
  if ((src>='0')&&(src<='9')) return 1;
  if (src=='_') return 1;
  if (src=='$') return 1;
  // Lots of other chars are identifier-legal in Javascript, but we won't go there.
  return 0;
}

static int js_measure_token(const char *src,int srcc) {
  if (srcc<1) return 0;

  if ((unsigned char)src[0]<=0x20) {
    int srcp=1;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    return srcp;
  }

  if ((srcc>=2)&&(src[0]=='/')) {
    if (src[1]=='/') {
      int srcp=2;
      while ((srcp<srcc)&&(src[srcp++]!=0x0a)) ;
      return srcp;
    }
    if (src[1]=='*') {
      int srcp=2;
      for (;;) {
        if (srcp>srcc-2) return 0;
        if ((src[srcp]=='*')&&(src[srcp+1]=='/')) return srcp+2;
        srcp++;
      }
    }
  }

  if ((src[0]=='"')||(src[0]=='\'')||(src[0]=='`')) {
    int srcp=1;
    for (;;) {
      if (srcp>=srcc) return 0;
      if (src[srcp]==src[0]) return srcp+1;
      if (src[srcp]=='\\') srcp+=2;
      else srcp+=1;
    }
  }

  if ((src[0]>='0')&&(src[0]<='9')) {
    int srcp=1;
    while ((srcp<srcc)&&(js_isident(src[srcp])||(src[srcp]=='.'))) srcp++;
    return srcp;
  }

  if (js_isident(src[0])) {
    int srcp=1;
    while ((srcp<srcc)&&js_isident(src[srcp])) srcp++;
    return srcp;
  }

  return 1;
}

static int js_token_is_space(const char *src,int srcc) {
  if (!src) return 1;
  if (srcc<1) return 1;
  if ((unsigned char)src[0]<=0x20) return 1;
  if ((srcc>=2)&&(src[0]=='/')) {
    if (src[1]=='/') return 1;
    if (src[1]=='*') return 1;
  }
  return 0;
}

// Includes reserved words and other conditionally reserved things, the broadest set of things that shouldn't be used as identifiers.
// Also, oddly "constructor" and "window" were not in the list, but ought to be.
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Lexical_grammar
static const char *js_keywords[]={
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

static int js_token_is_identifier(const char *src,int srcc) {
  if (!src||(srcc<1)) return 0;
  if (!js_isident(src[0])) return 0;
  if ((src[0]>='0')&&(src[0]<='9')) return 0;
  // Keywords are not "identifiers" for our purposes -- they must be false here.
  int lo=0,hi=sizeof(js_keywords)/sizeof(void*);
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int cmp=memcmp(src,js_keywords[ck],srcc);
    if (!cmp) {
      if (js_keywords[ck][srcc]) cmp=-1;
      else return 0;
    }
    if (cmp<0) hi=ck;
    else lo=ck+1;
  }
  return 1;
}

/* Output identifier, and a leading space if needed.
 */

static int js_emit_identifier(int seq) {
  if (min.dst.c&&js_isident(((char*)min.dst.v)[min.dst.c-1])) {
    if (sr_encode_u8(&min.dst,' ')) return -1;
  }
  char tmp[8];
  int tmpc=identifier_from_sequence(tmp,sizeof(tmp),seq);
  if ((tmpc<1)||(tmpc>sizeof(tmp))) return -1;
  if (sr_encode_raw(&min.dst,tmp,tmpc)<0) return -1;
  return 0;
}

/* Process an 'import' statement. Keyword is already consumed.
 */

static int js_import(const char *src,int srcc,const char *refname) {
  int srcp=0,complete=0;
  for (;;) {

    if (srcp>=srcc) return -1;
    if (src[srcp]==';') {
      if (!complete) return js_error(src,srcp,refname,"Malformed import. Must contain a string.");
      return srcp+1;
    }
  
    const char *token=src+srcp;
    int tokenc=js_measure_token(src+srcp,srcc-srcp);
    if (tokenc<1) return js_error(src,srcp,refname,"Tokenization failure around character '%c'",src[srcp]);
    srcp+=tokenc;
    if (js_token_is_space(token,tokenc)) continue;

    if ((tokenc>=2)&&((token[0]=='"')||(token[0]=='\'')||(token[0]=='`'))) {
      char subpath[1024];
      int subpathc=resolve_path(subpath,sizeof(subpath),refname,token+1,tokenc-2); // Import paths must not contain escapes.
      if ((subpathc<1)||(subpathc>=sizeof(subpath))) return js_error(src,srcp,refname,"Failed to resolve import");
      if (!imports_check(subpath,subpathc)) {
        char *serial=0;
        int serialc=file_read(&serial,subpath);
        if (serialc<0) return js_error(src,srcp,refname,"%s: Failed to read file",subpath);
        int err=minify_js_1(serial,serialc,0,subpath);
        free(serial);
        if (err<0) return err;
      }
      complete=1;
    }
  }
}

/* Minify one file of Javascript.
 */

static int minify_js_1(const char *src,int srcc,int require_closing_tag,const char *refname) {
  int srcp=0,closed=0;
  while (srcp<srcc) {

    if (require_closing_tag&&(srcp<=srcc-9)&&!memcmp(src+srcp,"</script>",9)) {
      srcp+=9;
      closed=1;
      break;
    }

    const char *token=src+srcp;
    int tokenc=js_measure_token(src+srcp,srcc-srcp);
    if (tokenc<1) return js_error(src,srcp,refname,"Tokenization failure around character '%c'",src[srcp]);
    srcp+=tokenc;

    if (js_token_is_space(token,tokenc)) {
      // Cool, skip it.

    } else if ((tokenc==6)&&!memcmp(token,"import",6)) {
      int err=js_import(src+srcp,srcc-srcp,refname);
      if (err<0) return err;
      srcp+=err;

    } else if ((tokenc==6)&&!memcmp(token,"export",6)) {
      // Skip the word "export", we're concatenating all imports.

    } else if ((tokenc==1)&&(token[0]=='.')) {
      // When a dot is followed by an identifier, the typical case, we turn it into a bracketted variable.
      token=src+srcp;
      tokenc=js_measure_token(src+srcp,srcc-srcp);
      if (tokenc<1) return js_error(src,srcp,refname,"Tokenization failure around character '%c'",src[srcp]);
      srcp+=tokenc;
      if (js_token_is_identifier(token,tokenc)) {
        int fresh=0;
        struct ident *ident=ident_intern(token,tokenc,&fresh);
        if (!ident) return -1;
        if (fresh) {
          //TODO
        }
        if (ident->preserve) {
          if (sr_encode_u8(&min.dst,'[')<0) return -1;
          if (js_emit_identifier(ident->seq)<0) return -1;
          if (sr_encode_u8(&min.dst,']')<0) return -1;
        } else {
          if (sr_encode_u8(&min.dst,'.')<0) return -1;
          if (js_emit_identifier(ident->seq)<0) return -1;
        }
      } else {
        if (sr_encode_u8(&min.dst,'.')<0) return -1;
        if (sr_encode_raw(&min.dst,token,tokenc)<0) return -1;
      }
      //TODO 2024-06-17T13:32: Identifiers are already a mess. I think we do need to use a proper AST.

    } else if (js_token_is_identifier(token,tokenc)) {
      int fresh=0;
      struct ident *ident=ident_intern(token,tokenc,&fresh);
      if (!ident) return -1;
      if (fresh) {
        //TODO Determine whether this identifier is replaceable. Set (ident->preserve=0) if so.
      }
      if (js_emit_identifier(ident->seq)<0) return -1;

    } else {
      // All other tokens pass through verbatim.
      if (sr_encode_raw(&min.dst,token,tokenc)<0) return -1;
    }
  }
  if (require_closing_tag&&!closed) return js_error(src,srcp,refname,"Expected '</script>'");
  if (sr_encode_u8(&min.dst,0x0a)<0) return -1;
  return srcp;
}

/* Minify Javascript, main entry point.
 */

int minify_js(const char *src,int srcc,int require_closing_tag,const char *refname) {
  imports_clear();
  idents_clear();
  if (sr_encode_raw(&min.dst,"<script>\n",-1)<0) return -1;
  int err=minify_js_1(src,srcc,require_closing_tag,refname);
  if (err<0) return err;
  if (sr_encode_raw(&min.dst,"</script>\n",-1)<0) return -1;
  return 0;
}
#endif

#if 0 /* "jsc", full transpiler. I got just a little ways in and came to my senses. Crazy to do all that. */
int minify_js(const char *src,int srcc,int require_closing_tag,const char *refname) {
  struct jsc_context ctx={0};
  int err=jsc_compile(&ctx,src,srcc,require_closing_tag,refname);
  if (err<0) {
    jsc_context_cleanup(&ctx);
    if (err!=-2) return js_error(src,0,refname,"Unspecified error processing Javascript text.");
    return -2;
  }
  srcc=err;
  if (sr_encode_raw(&min.dst,"<script>\n",-1)<0) return -1;
  if ((err=jsc_encode(&min.dst,&ctx))<0) {
    jsc_context_cleanup(&ctx);
    if (err!=-2) return js_error(src,0,refname,"Unspecified error rewriting Javascript.");
    return -2;
  }
  jsc_context_cleanup(&ctx);
  if (sr_encode_raw(&min.dst,"</script>\n",-1)<0) return -1;
  return srcc;
}
#endif

int minify_js(const char *src,int srcc,int require_closing_tag,const char *refname) {

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

  fprintf(stderr,"%s: TODO: Minify %d bytes of Javascript.\n",refname,srcc);

  return result;
}
