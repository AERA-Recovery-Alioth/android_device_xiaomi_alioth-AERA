/* SPDX-License-Identifier: Apache-2.0 */
/* Host-only unconditional policy query; never reads a running kernel policy. */
#include <stdio.h>
#include <stdlib.h>
#include <sepol/policydb/policydb.h>
#include <sepol/policydb/avtab.h>
int main(int argc,char **argv) {
 if(argc!=6) return 2;
 policydb_t policy; policy_file_t input;
 if(policydb_init(&policy)) return 2;
 policy_file_init(&input); input.type=PF_USE_STDIO; input.fp=fopen(argv[1],"rb");
 if(!input.fp || policydb_read(&policy,&input,0)) { fprintf(stderr,"policy read failed\n"); return 2; }
 type_datum_t *source=hashtab_search(policy.p_types.table,argv[2]), *target=hashtab_search(policy.p_types.table,argv[3]);
 class_datum_t *object_class=hashtab_search(policy.p_classes.table,argv[4]);
 if(!source || !target || !object_class) { fprintf(stderr,"lookup source=%p target=%p class=%p\n",(void*)source,(void*)target,(void*)object_class);return 2;}
 perm_datum_t *permission=hashtab_search(object_class->permissions.table,argv[5]);
 if(!permission && object_class->comdatum) permission=hashtab_search(object_class->comdatum->permissions.table,argv[5]);
 if(!permission) { fprintf(stderr,"permission missing\n");return 2;}
 ebitmap_node_t *source_node,*target_node; unsigned source_bit,target_bit; int allowed=0;
 ebitmap_for_each_positive_bit(&policy.type_attr_map[source->s.value-1],source_node,source_bit) {
  ebitmap_for_each_positive_bit(&policy.type_attr_map[target->s.value-1],target_node,target_bit) {
   avtab_key_t key={.source_type=source_bit+1,.target_type=target_bit+1,.target_class=object_class->s.value,.specified=AVTAB_ALLOWED};
   avtab_datum_t *rule=avtab_search(&policy.te_avtab,&key);
   if(rule && (rule->data & (1U << (permission->s.value-1)))) allowed=1;
  }
 }
 printf("%s %s:%s %s %s\n",argv[2],argv[3],argv[4],argv[5],allowed?"ALLOW":"DENY");
 fclose(input.fp);policydb_destroy(&policy);return allowed?0:1;
}