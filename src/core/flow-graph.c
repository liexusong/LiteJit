#include "flow-graph.h"

#define _LJIT_LABEL_FG_INIT(size)                       \
    _ljit_label_fg_node(1, 0, 0, size, NULL, NULL)

#define _LJIT_LABEL_FG_FREE()                           \
    _ljit_label_fg_node(0, 1, 0, 0, NULL, NULL)

#define _LJIT_LABEL_FG_GET(label, fun)                  \
    _ljit_label_fg_node(0, 0, 0, 0, label, fun)

#define _LJIT_LABEL_FG_GET_INDEX(index, fun)                  \
    _ljit_label_fg_node(0, 0, 1, index, NULL, fun)

static int _ljit_fg_index()
{
    static int i = -1;

    ++i;

    return i;
}

static ljit_flow_graph *_ljit_label_fg_node(unsigned int init,
                                            unsigned int destroy,
                                            unsigned int set,
                                            unsigned int number_tmp,
                                            ljit_label *l,
                                            ljit_function *fun)
{
    static unsigned int size = 0;
    static ljit_flow_graph ** fg = NULL;

    if (init)
    {
        size = number_tmp;
        fg = calloc(1, size * sizeof(ljit_flow_graph *));
        return NULL;
    }
    else if (destroy)
    {
        free(fg);
        return NULL;
    }
    else
    {
        unsigned int label_num = 0;
        ljit_block *b = NULL;

        if (set)
        {
            label_num = number_tmp;
            b = ljit_get_block_from_label_num(fun, label_num);
        }
        else
        {
            label_num = l->index;
            b = ljit_get_block_from_label(fun, l);
        }

        if (!fg[label_num])
        {
            if (!b)
                return NULL;

            fg[label_num] = _ljit_new_flow_graph(b->instrs->head->instr);

            if (!fg[label_num])
                return NULL;
        }

        return fg[label_num];
    }
}

ljit_flow_graph *_ljit_new_flow_graph(ljit_bytecode *instr)
{
    ljit_flow_graph *fg = NULL;

    if ((fg = malloc(sizeof(ljit_flow_graph))) == NULL)
        return NULL;

    fg->instr = instr;
    fg->first_next = NULL;
    fg->second_next = NULL;
    fg->index = _ljit_fg_index();
    fg->marked = 1;

    return fg;
}

void _ljit_free_flow_graph(ljit_flow_graph *fg)
{
    if (!fg)
        return;

    _ljit_free_flow_graph(fg->first_next);
    _ljit_free_flow_graph(fg->second_next);
    free(fg);
}

static ljit_flow_graph *
_ljit_recurse_build_fg(ljit_block *blk,
                       struct _ljit_bytecode_list_element_s *tmp_instr,
                       ljit_function *fun)
{
    ljit_flow_graph *fg = NULL;
    ljit_flow_graph *second = NULL;

    if (!tmp_instr)
    {
        if (!blk->next)
            return NULL;
        else
            return _ljit_recurse_build_fg(blk->next, blk->next->instrs->head,
                                          fun);
    }

    switch (tmp_instr->instr->type)
    {
        case JUMP:
            {
                ljit_block *b = ljit_get_block_from_label(fun,
                                                          (ljit_label*)tmp_instr->instr->op1);

                if ((fg = _ljit_new_flow_graph(tmp_instr->instr)) == NULL)
                    goto error;

                fg->first_next = _ljit_recurse_build_fg(b, b->instrs->head,
                                                        fun);

                return fg;
            }
            break;
        case JUMP_IF:
        case JUMP_IF_NOT:
            {
                ljit_block *b = ljit_get_block_from_label(fun,
                                                          (ljit_label*)tmp_instr->instr->op2);
                if ((fg = _ljit_new_flow_graph(tmp_instr->instr)) == NULL)
                    goto error;

                second = _ljit_recurse_build_fg(b, b->instrs->head, fun);
            }
            break;
        case LABEL:
            fg = _LJIT_LABEL_FG_GET_INDEX(*(ljit_int*)tmp_instr->instr->op1,
                                         fun);
            break;
        default:
            if ((fg = _ljit_new_flow_graph(tmp_instr->instr)) == NULL)
                goto error;
            break;
    }

    fg->first_next = _ljit_recurse_build_fg(blk, tmp_instr->next, fun);
    fg->second_next = second;

    return fg;

error:
    free(fg);
    return NULL;
}

ljit_flow_graph *_ljit_build_flow_graph(ljit_function *fun)
{
    ljit_flow_graph *fg = NULL;

    _LJIT_LABEL_FG_INIT(fun->lbl_index);

    fg = _ljit_recurse_build_fg(fun->start_blk,
                                fun->start_blk->instrs->head,
                                fun);

    _LJIT_LABEL_FG_FREE();

    _ljit_unmark_flow_graph(fg);

    return fg;
}

void _ljit_unmark_flow_graph(ljit_flow_graph *fg)
{
    if (!fg || !fg->marked)
        return;

    fg->marked = 0;

    _ljit_unmark_flow_graph(fg->first_next);
    _ljit_unmark_flow_graph(fg->second_next);
}

static void _ljit_dot_write_label(FILE *f, ljit_flow_graph *fg)
{
    if (!fg || fg->marked)
        return;

    fg->marked = 1;

    /* Generate the name of the label */
    fprintf(f, "label%u [label = \"", fg->index);

    /* Dump instruction of the node */
    _ljit_dump_instr(f, fg->instr);
    fprintf(f, "\"];\n");

    _ljit_dot_write_label(f, fg->first_next);
    _ljit_dot_write_label(f, fg->second_next);
}

static void _ljit_dot_write_edges(FILE *f, ljit_flow_graph *fg)
{
    if (!fg || fg->marked)
        return;

    fg->marked = 1;

    if (fg->first_next)
    {
        fprintf(f, "label%u -> label%u;\n", fg->index, fg->first_next->index);
        _ljit_dot_write_edges(f, fg->first_next);
    }

    if (fg->second_next)
    {
        fprintf(f, "label%u -> label%u;\n", fg->index, fg->second_next->index);
        _ljit_dot_write_edges(f, fg->second_next);
    }
}

int _ljit_dot_flow_graph(ljit_flow_graph *fg, const char *name)
{
    FILE *f = NULL;

    if ((f = fopen(name, "w")) == NULL)
        return -1;

    fprintf(f, "digraph flow_graph {\n");

    /* Write all node content */
    _ljit_dot_write_label(f, fg);
    _ljit_unmark_flow_graph(fg);

    fprintf(f, "\n");

    /* Write the flow graph edges */
    _ljit_dot_write_edges(f, fg);
    _ljit_unmark_flow_graph(fg);

    fprintf(f, "}");

    fprintf(f, "\n");

    fclose(f);

    return 0;
}