#include "min_internal.h"
#include "jsc.h"

/* Delete.
 */
 
void jsc_node_del(struct jsc_node *node) {
  if (!node) return;
  if (node->childv) {
    while (node->childc-->0) jsc_node_del(node->childv[node->childc]);
    free(node->childv);
  }
  if (node->text) free(node->text);
  free(node);
}

/* New.
 */
 
struct jsc_node *jsc_node_new() {
  struct jsc_node *node=calloc(1,sizeof(struct jsc_node));
  if (!node) return 0;
  return node;
}

/* Spawn.
 */
 
struct jsc_node *jsc_node_spawn(struct jsc_node *parent,const char *text,int textc) {
  struct jsc_node *node=calloc(1,sizeof(struct jsc_node));
  if (!node) return 0;
  if (jsc_node_set_text(node,text,textc)<0) {
    jsc_node_del(node);
    return 0;
  }
  if (jsc_node_add_child(parent,node)<0) {
    jsc_node_del(node);
    return 0;
  }
  return node;
}

/* Set text.
 */
 
int jsc_node_set_text(struct jsc_node *node,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (node->text) free(node->text);
  node->text=nv;
  node->textc=srcc;
  return 0;
}

/* Add child.
 */
 
int jsc_node_add_child(struct jsc_node *parent,struct jsc_node *child) {
  if (!parent||!child) return -1;
  if (child->parent==parent) return 0;
  if (child->parent) return -1;
  if (parent->childc>=parent->childa) {
    int na=parent->childa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(parent->childv,sizeof(void*)*na);
    if (!nv) return -1;
    parent->childv=nv;
    parent->childa=na;
  }
  parent->childv[parent->childc++]=child;
  child->parent=parent;
  return 0;
}

/* Encode.
 */
 
int jsc_node_encode(struct sr_encoder *dst,struct jsc_node *node) {
  //TODO
  if (sr_encode_raw(dst,node->text,node->textc)<0) return -1;
  int i=0,err; for (;i<node->childc;i++) if ((err=jsc_node_encode(dst,node->childv[i]))<0) return err;
  return 0;
}
