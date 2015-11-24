"""Analyze the problem in sum-of-prox form and combine/split terms."""

import logging

from collections import defaultdict

from epsilon import expression
from epsilon import tree_format
from epsilon.compiler.problem_graph import *
from epsilon.compiler import validate
from epsilon.compiler.transforms import linear
from epsilon.expression_pb2 import Expression, ProxFunction
from epsilon.expression_util import fp_expr

Prox = ProxFunction

def has_data_constant(expr):
    if (expr.expression_type == Expression.CONSTANT and
        expr.constant.data_location != ""):
        return True

    for arg in expr.arg:
        if has_data_constant(arg):
            return True

    return False

def prox_op(expr):
    if expr.expression_type == Expression.ADD:
        return prox_op(expr.arg[0])
    if expr.expression_type == Expression.MULTIPLY:
        return prox_op(expr.arg[1])
    return expr.proximal_operator.name

def is_prox_friendly_constraint(graph, f):
    """Returns true if f represents a prox-friendly equality constraint.

    In other words, one that can be treated as a constraint without interfering
    with the proximal operators for the other objective terms."""

    if not f.expr.prox_function.prox_function_type == Prox.ZERO:
        return False

    assert len(f.expr.arg) == 1
    for f_var in graph.edges_by_function[f]:
        edges = graph.edges_by_variable[f_var.variable]
        if (len(edges) > 1 and
            not f.expr.arg[0].affine_props.linear_maps[f_var.variable].scalar):
            return False

    return True

def max_overlap_function(graph, f):
    """Return the objective term with maximum overlap in variables."""

    def variables(g):
        return set(g_var.variable for g_var in graph.edges_by_function[g])
    variables_f = variables(f)
    def overlap(g):
        return len(variables(g).intersection(variables_f))

    candidates = [g for g in graph.obj_terms if g != f]
    if not candidates:
        return

    h = max(candidates, key=overlap)

    # Only return a function if there is some overlap
    if overlap(h):
        return h

def separate_var(f_var):
    variable_id = "separate:%s:%s" % (
        f_var.variable, fp_expr(f_var.function.expr))
    return Expression(
        expression_type=Expression.VARIABLE,
        variable=Variable(variable_id=variable_id),
        size=f_var.instances[0].size)

def combine_affine_functions(graph):
    """Combine affine functions with other objective terms."""
    for f in graph.obj_terms:
        if not f.expr.prox_function.prox_function_type == Prox.AFFINE:
            continue

        # no variables
        if not graph.edges_by_function[f]:
            continue

        # no other functions with overlap
        g = max_overlap_function(graph, f)
        if not g:
            continue

        graph.remove_function(f)
        graph.remove_function(g)

        # Combine functions with non-affine function first
        graph.add_function(
            Function(expression.add(g.expr, f.expr), constraint=False))

def move_equality_indicators(graph):
    """Move certain equality indicators from objective to constraints."""
    # Single prox case, dont move it
    if len(graph.obj_terms) == 1:
        return

    for f in graph.obj_terms:
        if is_prox_friendly_constraint(graph, f):
            # Modify it to be an equality constraint
            f.expr.CopyFrom(expression.indicator(Cone.ZERO, f.expr.arg[0]))
            f.constraint = True

def separate_objective_terms(graph):
    """Add variable copies to make functions separable.

    This applies to objective functions only and we dont need to modify the
    first occurence.
   """
    for var in graph.variables:
        # Exclude constraint terms
        f_vars = [f_var for f_var in graph.edges_by_variable[var]
                  if not f_var.function.constraint]

        skipped_one = False
        for f_var in reversed(f_vars):
            # Skip first one, rename the rest
            if not f_var.has_linops() and not skipped_one:
                skipped_one = True
                continue

            graph.remove_edge(f_var)

            new_var_id = "separate:%s:%s" % (
                f_var.variable, fp_expr(f_var.function.expr))
            old_var, new_var = f_var.replace_variable(new_var_id)
            graph.add_function(
                Function(
                    linear.transform_expr(
                        expression.eq_constraint(old_var, new_var)),
                    constraint=True))
            graph.add_edge(f_var)

def add_null_prox(graph):
    """Add f(x) = 0 term for variables only appearing in constraints."""

    for var in graph.variables:
        f_vars = [f_var for f_var in graph.edges_by_variable[var]
                  if not f_var.function.constraint]

        if f_vars:
            continue

        var_expr = graph.edges_by_variable[var][0].instances[0]
        f_expr = expression.prox_function(
            ProxFunction(prox_function_type=Prox.CONSTANT), var_expr)
        graph.add_function(Function(f_expr, constraint=False))


# TODO(mwytock): Add back these optimizations when ready
# combine_affine_functions
# move_equality_indicators,

GRAPH_TRANSFORMS = [
    separate_objective_terms,
    add_null_prox,
]

def transform_problem(problem):
    graph = ProblemGraph(problem)
    if not graph.variables:
        return problem

    for f in GRAPH_TRANSFORMS:
        f(graph)
        # logging.debug(
        #     "%s:\n%s",
        #     f.__name__,
        #     tree_format.format_problem(graph.problem()))
    return graph.problem()
