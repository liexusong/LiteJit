#include <ljit/bytecode.h>
#include "internal.h"

static ljit_bytecode *_ljit_new_bytecode(ljit_bytecode_type type,
                                         ljit_value op1,
                                         ljit_value op2)
{
    ljit_bytecode *instr = NULL;

    if ((instr = malloc(sizeof(ljit_bytecode))) == NULL)
        return NULL;

    instr->type = type;
    instr->op1 = op1;
    instr->op2 = op2;
    instr->ret_val = NULL;

    return instr;
}

ljit_value ljit_inst_get_param(ljit_function *fun, ljit_uchar pos)
{
    ljit_value ret_val = NULL;
    ljit_bytecode *instr = NULL;
    ljit_value pos_cst = NULL;

    /* Allocate new constant that represent the number of the parameter */
    if ((pos_cst = ljit_new_uchar_cst(pos)) == NULL)
        return NULL;

    if ((instr = _ljit_new_bytecode(GET_PARAM, pos_cst, NULL)) == NULL)
    {
        ljit_free_value(pos_cst);
        return NULL;
    }

    if ((ret_val = _ljit_new_temporary(fun,
                                       fun->signature->params_type[pos])) == NULL)
    {
        ljit_free_value(pos_cst);
        _ljit_free_bytecode(instr);
        return NULL;
    }

    instr->ret_val = ret_val;
    ljit_bytecode_list_add(&fun->bytecode, instr);

    return ret_val;
}

/* FIXME : Check return type and the value type */
int ljit_inst_return(ljit_function *fun, ljit_value val)
{
    ljit_bytecode *instr = NULL;

    if ((instr = _ljit_new_bytecode(RETURN, val, NULL)) == NULL)
        return -1;

    ljit_bytecode_list_add(&fun->bytecode, instr);

    return 0;
}

/* FIXME : Type check */
ljit_value ljit_inst_mul(ljit_function *fun, ljit_value op1, ljit_value op2)
{
    ljit_value ret_val = NULL;
    ljit_bytecode *instr = NULL;

    if ((instr = _ljit_new_bytecode(MUL, op1, op2)) == NULL)
        return NULL;

    if ((ret_val = _ljit_new_temporary(fun, op1->type)) == NULL)
    {
        _ljit_free_bytecode(instr);
        return NULL;
    }

    instr->ret_val = ret_val;
    ljit_bytecode_list_add(&fun->bytecode, instr);

    return ret_val;
}
