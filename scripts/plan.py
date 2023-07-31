#!/usr/bin/env python3

import sys
from unified_planning.io import PDDLReader
from unified_planning.shortcuts import *


def main():
    if len(sys.argv) != 5:
        exit(1)

    planner, domain_filename, problem_filename, plan_filename = sys.argv[1:5]

    # disable printing of planning engine credits
    up.shortcuts.get_environment().credits_stream = None
    
    reader = PDDLReader()
    problem = reader.parse_problem(domain_filename, problem_filename)

    try:
        with OneshotPlanner(name=planner, problem_kind=problem.kind) as planner:
            result = planner.solve(problem)
            if result.status == up.engines.PlanGenerationResultStatus.SOLVED_SATISFICING or \
                    result.status == up.engines.PlanGenerationResultStatus.SOLVED_OPTIMALLY:
                plan = result.plan
                if plan is not None and isinstance(plan, up.plans.time_triggered_plan.TimeTriggeredPlan):
                    with open(plan_filename, "w") as plan_file:
                        for start, action, duration in plan.timed_actions:
                            action_repr = f"({action.action.name} {' '.join([str(p) for p in action.actual_parameters])})" # TODO: use p.name if available
                            plan_file.write("%s: %s [%s]\n" % (
                                float(start), action_repr, float(duration)))
        exit(0)

    except up.exceptions.UPNoRequestedEngineAvailableException:
        print(f"Planner '{planner}' is not available")
        exit(1)


if __name__ == '__main__':
    main()
