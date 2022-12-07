#!/usr/bin/env python3


import sys
from unified_planning.io import PDDLReader
from unified_planning.shortcuts import *


def main():
    if len(sys.argv) != 5:
        exit(1)

    planner, domain_filename, problem_filename, plan_filename = sys.argv[1:5]

    reader = PDDLReader()
    problem = reader.parse_problem(domain_filename, problem_filename)

    # TODO: check if planner exists
    with OneshotPlanner(name=planner, problem_kind=problem.kind) as planner:
        result = planner.solve(problem)
        plan = result.plan
        if plan is not None:
            with open(plan_filename, "w") as plan_file:
                for start, action, duration in plan.timed_actions:
                    action_repr = f"({action.action.name} {' '.join([str(p) for p in action.actual_parameters])})"
                    plan_file.write("%s: %s [%s]\n" % (
                        float(start), action_repr, float(duration)))

    exit(0)


if __name__ == '__main__':
    main()
