#include "gwion_util.h"
#include "gwion_ast.h"
#include "gwion_env.h"
#include "vm.h"
#include "gwion.h"
#define __CLEAN_IMPL
#include "clean.h"

ANN static void clean_array_sub(Clean *a, Array_Sub b) {
  if(b->exp)
    clean_exp(a, b->exp);
}

ANN static void clean_id_list(Clean *a, ID_List b) {
  if(b->next)
    clean_id_list(a, b->next);
}

ANN static void clean_type_list(Clean *a, Type_List b) {
  clean_type_decl(a, b->td);
  if(b->next)
    clean_type_list(a, b->next);
}

ANN static void clean_tmpl(Clean *a, Tmpl *b) {
  if(b->list)
    clean_id_list(a, b->list);
  if(b->call)
    clean_type_list(a, b->call);
}

ANN static void clean_range(Clean *a, Range *b) {
  if(b->start)
    clean_exp(a, b->start);
  if(b->end)
    clean_exp(a, b->end);
}

ANN static void clean_type_decl(Clean *a, Type_Decl *b) {
  if(b->exp)
    clean_exp(a, b->exp);
  if(b->array)
    clean_array_sub(a, b->array);
  if(b->types)
    clean_type_list(a, b->types);
  if(b->next)
    clean_type_decl(a, b->next);
}

ANN static void clean_prim(Clean *a, Exp_Primary *b) {
  if(b->prim_type == ae_prim_hack   ||
     b->prim_type == ae_prim_typeof ||
     b->prim_type == ae_prim_interp)
    clean_exp(a, b->d.exp);
  else if(b->prim_type == ae_prim_array)
    clean_array_sub(a, b->d.array);
  else if(b->prim_type == ae_prim_range)
    clean_range(a, b->d.range);
}

ANN static void clean_var_decl(Clean *a, Var_Decl b) {
  if(b->array)
    clean_array_sub(a, b->array);
  if(a->scope && b->value)
    REM_REF(b->value, a->gwion)
}

ANN static void clean_var_decl_list(Clean *a, Var_Decl_List b) {
  clean_var_decl(a, b->self);
  if(b->next)
    clean_var_decl_list(a, b->next);
}

ANN static void clean_exp_decl(Clean *a, Exp_Decl *b) {
  if(b->td)
    clean_type_decl(a, b->td);
  clean_var_decl_list(a, b->list);
}

ANN static void clean_exp_binary(Clean *a, Exp_Binary *b) {
  clean_exp(a, b->lhs);
  clean_exp(a, b->rhs);
}

ANN static void clean_exp_unary(Clean *a, Exp_Unary *b) {
  if(b->exp)
    clean_exp(a, b->exp);
  if(b->td)
    clean_type_decl(a, b->td);
  if(b->code)
    clean_stmt(a, b->code);
}

ANN static void clean_exp_cast(Clean *a, Exp_Cast *b) {
  clean_type_decl(a, b->td);
  clean_exp(a, b->exp);
}

ANN static void clean_exp_post(Clean *a, Exp_Postfix *b) {
  clean_exp(a, b->exp);
}

ANN static void clean_exp_call(Clean *a, Exp_Call *b) {
  clean_exp(a, b->func);
  if(b->args)
    clean_exp(a, b->args);
  if(b->tmpl)
    clean_tmpl(a, b->tmpl);
}

ANN static void clean_exp_array(Clean *a, Exp_Array *b) {
  clean_exp(a, b->base);
  clean_array_sub(a, b->array);
}

ANN static void clean_exp_slice(Clean *a, Exp_Slice *b) {
  clean_exp(a, b->base);
  clean_range(a, b->range);
}

ANN static void clean_exp_if(Clean *a, Exp_If *b) {
  clean_exp(a, b->cond);
  if(b->if_exp)
    clean_exp(a, b->if_exp);
  clean_exp(a, b->else_exp);
}

ANN static void clean_exp_dot(Clean *a, Exp_Dot *b) {
  clean_exp(a, b->base);
}

ANN static void clean_exp_lambda(Clean *a, Exp_Lambda *b) {
  clean_func_def(a, b->def);
}

DECL_EXP_FUNC(clean, void, Clean*)
ANN static void clean_exp(Clean *a, Exp b) {
  clean_exp_func[b->exp_type](a, &b->d);
  if(b->next)
    clean_exp(a, b->next);
}

ANN static void clean_stmt_exp(Clean *a, Stmt_Exp b) {
  if(b->val)
    clean_exp(a, b->val);
}

ANN static void clean_stmt_flow(Clean *a, Stmt_Flow b) {
  ++a->scope;
  clean_exp(a, b->cond);
  clean_stmt(a, b->body);
  --a->scope;
}

#define clean_stmt_while clean_stmt_flow
#define clean_stmt_until clean_stmt_flow

ANN static void clean_stmt_for(Clean *a, Stmt_For b) {
  ++a->scope;
  clean_stmt(a, b->c1);
  if(b->c2)
    clean_stmt(a, b->c2);
  if(b->c3)
    clean_exp(a, b->c3);
  clean_stmt(a, b->body);
  --a->scope;
}

ANN static void clean_stmt_each(Clean *a, Stmt_Each b) {
  ++a->scope;
  clean_exp(a, b->exp);
  clean_stmt(a, b->body);
  if(b->v)
    REM_REF(b->v, a->gwion)
  --a->scope;
}

ANN static void clean_stmt_loop(Clean *a, Stmt_Loop b) {
  ++a->scope;
  clean_exp(a, b->cond);
  clean_stmt(a, b->body);
  --a->scope;
}

ANN static void clean_stmt_if(Clean *a, Stmt_If b) {
  ++a->scope;
  clean_exp(a, b->cond);
  clean_stmt(a, b->if_body);
  if(b->else_body)
    clean_stmt(a, b->else_body);
  --a->scope;
}

ANN static void clean_stmt_code(Clean *a, Stmt_Code b) {
  ++a->scope;
  if(b->stmt_list)
    clean_stmt_list(a, b->stmt_list);
  --a->scope;
}

ANN static void clean_stmt_varloop(Clean *a, Stmt_VarLoop b) {
  ++a->scope;
  clean_exp(a, b->exp);
  clean_stmt(a, b->body);
  --a->scope;
}

ANN static void clean_stmt_return(Clean *a, Stmt_Exp b) {
  if(b->val)
    clean_exp(a, b->val);
}

ANN static void clean_case_list(Clean *a, Stmt_List b) {
  clean_stmt_case(a, &b->stmt->d.stmt_match);
  if(b->next)
    clean_case_list(a, b->next);
}

ANN static void clean_stmt_match(Clean *a, Stmt_Match b) {
  ++a->scope;
  clean_exp(a, b->cond);
  clean_case_list(a, b->list);
  if(b->where)
    clean_stmt(a, b->where);
  --a->scope;
}

ANN static void clean_stmt_case(Clean *a, Stmt_Match b) {
  ++a->scope;
  clean_exp(a, b->cond);
  clean_stmt_list(a, b->list);
  if(b->when)
    clean_exp(a, b->when);
  --a->scope;
}


ANN static void clean_dummy(Clean *a NUSED, void *b NUSED) {}
#define clean_stmt_jump clean_dummy
#define clean_stmt_pp clean_dummy
#define clean_stmt_break clean_dummy
#define clean_stmt_continue clean_dummy

DECL_STMT_FUNC(clean, void, Clean*)
ANN static void clean_stmt(Clean *a, Stmt b) {
  clean_stmt_func[b->stmt_type](a, &b->d);
}

ANN static void clean_arg_list(Clean *a, Arg_List b) {
  if(b->td)
    clean_type_decl(a, b->td);
  clean_var_decl(a, b->var_decl);
  if(b->next)
    clean_arg_list(a, b->next);
}

ANN static void clean_decl_list(Clean *a, Decl_List b) {
  clean_exp(a, b->self);
  if(b->next)
    clean_decl_list(a, b->next);
}

ANN static void clean_stmt_list(Clean *a, Stmt_List b) {
  clean_stmt(a, b->stmt);
  if(b->next)
    clean_stmt_list(a, b->next);
}

ANN static void clean_func_base(Clean *a, Func_Base *b) {
  if(b->td)
    clean_type_decl(a, b->td);
  if(b->args)
    clean_arg_list(a, b->args);
  if(b->tmpl)
    clean_tmpl(a, b->tmpl);
}

ANN static void clean_func_def(Clean *a, Func_Def b) {
  clean_func_base(a, b->base);
  ++a->scope;
  if(b->d.code && !GET_FLAG(b->base, builtin))
    clean_stmt(a, b->d.code);
  else
    b->d.code = NULL;
  --a->scope;
}

ANN void func_def_cleaner(const Gwion gwion, Func_Def b) {
  Clean a = { .gwion=gwion };
  clean_func_def(&a, b);
  free_func_def(gwion->mp, b);
}

ANN static void clean_class_def(Clean *a, Class_Def b) {
  clean_type_def(a, &b->base);
  if(b->body)
    clean_ast(a, b->body);
}

ANN void class_def_cleaner(const Gwion gwion, Class_Def b) {
  Clean a = { .gwion=gwion };
  clean_class_def(&a, b);
  free_class_def(gwion->mp, b);
}

ANN static void clean_enum_def(Clean *a, Enum_Def b) {
  clean_id_list(a, b->list);
}

ANN static void clean_union_def(Clean *a, Union_Def b) {
  clean_decl_list(a, b->l);
  if(b->tmpl)
    clean_tmpl(a, b->tmpl);
}

ANN static void clean_fptr_def(Clean *a, Fptr_Def b) {
  clean_func_base(a, b->base);
  if(b->type)
    REM_REF(b->type, a->gwion)
}

ANN static void clean_type_def(Clean *a, Type_Def b) {
  if(b->ext)
    clean_type_decl(a, b->ext);
  if(b->tmpl)
    clean_tmpl(a, b->tmpl);
}

DECL_SECTION_FUNC(clean, void, Clean*)

ANN static inline void clean_section(Clean *a, Section *b) {
  clean_section_func[b->section_type](a, *(void**)&b->d);
}

ANN static void clean_ast(Clean *a, Ast b) {
  clean_section(a, b->section);
  if(b->next)
    clean_ast(a, b->next);
}

ANN void ast_cleaner(const Gwion gwion, Ast b) {
  Clean a = { .gwion=gwion };
  clean_ast(&a, b);
  free_ast(gwion->mp, b);
}
