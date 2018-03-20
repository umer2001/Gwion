#include "type.h"
#include "err_msg.h"
#include "instr.h"
#include "import.h"

#define TEST0(pos) if(!*(m_int*)REG(pos))Except(shred, "ZeroDivideException")
#define PREFIX int

#define _describe(prefix, sz, name, action, ...) \
static INSTR(prefix##_##name) { GWDEBUG_EXE \
  POP_REG(shred, sz); \
   __VA_ARGS__ \
  action \
}

#define describe(name, op, ...) _describe(int, SZ_INT, name, \
  {*(m_int*)REG(-SZ_INT) op##= *(m_int*)REG(0); }, __VA_ARGS__)

#define describe_r(name, op, ...) _describe(int_r, SZ_INT, name, \
  {*(m_int*)REG(-SZ_INT) = (**(m_int**)REG(0) op##= (*(m_int*)REG(-SZ_INT)));}, __VA_ARGS__)

#define describe_logical(name, op) _describe(int, SZ_INT, name, \
  {*(m_int*)REG(-SZ_INT) = (*(m_int*)REG(-SZ_INT) op *(m_int*)REG(0));},)
#define describe_pre(name, op) \
static INSTR(int_pre_##name)  { *(m_int*)REG(- SZ_INT) = op(**(m_int**)REG(- SZ_INT)); }
#define describe_post(name, op) \
static INSTR(int_post_##name) { GWDEBUG_EXE *(m_int*)REG(- SZ_INT) = (**(m_int**)REG(- SZ_INT))op; }

static INSTR(int_assign) { GWDEBUG_EXE
  POP_REG(shred, SZ_INT);
  *(m_int*)REG(-SZ_INT) = (**(m_int**)REG(-SZ_INT) = *(m_int*)REG(0));
}

static INSTR(int_negate) { GWDEBUG_EXE *(m_int*)REG(-SZ_INT) *= -1; }

INSTR(int_not) { GWDEBUG_EXE *(m_int*)REG(-SZ_INT) = !*(m_int*)REG(-SZ_INT); }

static INSTR(int_r_assign) { GWDEBUG_EXE
  POP_REG(shred, SZ_INT);
  **(m_int**)REG(0) = *(m_int*)REG(-SZ_INT);
}

describe(plus,   +,)
describe(minus,  -,)
describe(times,  *,)
describe(divide, /, TEST0(0))
describe(modulo, %, TEST0(0))

describe_logical(and,  &&)
describe_logical(or,   ||)
describe_logical(eq,   ==)
describe_logical(neq,  !=)
describe_logical(gt,    >)
describe_logical(ge,   >=)
describe_logical(lt,    <)
describe_logical(le,   <=)
describe_logical(sl,   <<)
describe_logical(sr,   >>)
describe_logical(sand,  &)
describe_logical(sor,   |)
describe_logical(xor,   ^)

describe_pre(inc, ++)
describe_pre(dec, --)
describe_post(inc, ++)
describe_post(dec, --)

describe_r(plus,   +,)
describe_r(minus,  -,)
describe_r(times,  *,)
describe_r(divide, /, TEST0(-SZ_INT))
describe_r(modulo, %, TEST0(-SZ_INT))
describe_r(sl,     <<,)
describe_r(sr,     >>,)
describe_r(sand,   &,)
describe_r(sor,    |,)
describe_r(sxor,   ^,)

#define CHECK_OP(op, check, func) _CHECK_OP(op, check, int_##func)

m_bool import_int(Gwi gwi) {
  CHECK_BB(gwi_oper_ini(gwi, "int", "int", "int"))
  CHECK_BB(gwi_oper_add(gwi, opck_assign))
  CHECK_BB(gwi_oper_end(gwi, op_assign,       int_assign))
  CHECK_BB(gwi_oper_end(gwi, op_plus,         int_plus))
  CHECK_BB(gwi_oper_end(gwi, op_minus,        int_minus))
  CHECK_BB(gwi_oper_end(gwi, op_times,        int_times))
  CHECK_BB(gwi_oper_end(gwi, op_divide,       int_divide))
  CHECK_BB(gwi_oper_end(gwi, op_percent,      int_modulo))
  CHECK_BB(gwi_oper_end(gwi, op_and,          int_and))
  CHECK_BB(gwi_oper_end(gwi, op_or,           int_or))
  CHECK_BB(gwi_oper_end(gwi, op_eq,           int_eq))
  CHECK_BB(gwi_oper_end(gwi, op_neq, 		  int_neq))
  CHECK_BB(gwi_oper_end(gwi, op_gt,           int_gt))
  CHECK_BB(gwi_oper_end(gwi, op_ge, 	      int_ge))
  CHECK_BB(gwi_oper_end(gwi, op_lt, 		  int_lt))
  CHECK_BB(gwi_oper_end(gwi, op_le, 	      int_le))
  CHECK_BB(gwi_oper_end(gwi, op_shift_right,  int_sr))
  CHECK_BB(gwi_oper_end(gwi, op_shift_left,   int_sl))
  CHECK_BB(gwi_oper_end(gwi, op_s_and,        int_sand))
  CHECK_BB(gwi_oper_end(gwi, op_s_or,         int_sor))
  CHECK_BB(gwi_oper_end(gwi, op_s_xor, 	      int_xor))
  CHECK_OP(chuck,        rassign, r_assign)
  CHECK_OP(plus_chuck,   rassign, r_plus)
  CHECK_OP(minus_chuck,  rassign, r_minus)
  CHECK_OP(times_chuck,  rassign, r_times)
  CHECK_OP(divide_chuck, rassign, r_divide)
  CHECK_OP(modulo_chuck, rassign, r_modulo)
  CHECK_OP(rsl,          rassign, r_sl)
  CHECK_OP(rsr,          rassign, r_sr)
  CHECK_OP(rsand,        rassign, r_sand)
  CHECK_OP(rsor,         rassign, r_sor)
  CHECK_OP(rsxor,        rassign, r_sxor)
  CHECK_BB(gwi_oper_ini(gwi, NULL, "int", "int"))
  CHECK_BB(gwi_oper_add(gwi,  opck_unary_meta))
  CHECK_BB(gwi_oper_end(gwi,  op_minus,       int_negate))
  CHECK_BB(gwi_oper_add(gwi,  opck_unary_meta))
  CHECK_BB(gwi_oper_end(gwi,  op_exclamation, int_not))
  CHECK_OP(plusplus,   unary, pre_inc)
  CHECK_OP(minusminus, unary, pre_dec)
  CHECK_BB(gwi_oper_ini(gwi, "int", NULL, "int"))
  CHECK_OP(plusplus,   post,  post_inc)
  CHECK_OP(minusminus, post,  post_dec)
  return 1;
}
