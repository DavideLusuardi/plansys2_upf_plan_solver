// Copyright 2019 Intelligent Robotics Lab
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <ament_index_cpp/get_package_share_directory.hpp>

#include "plansys2_msgs/msg/plan_item.hpp"
#include "plansys2_upf_plan_solver/upf_plan_solver.hpp"
#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(plansys2::UPFPlanSolver, plansys2::PlanSolverBase);

namespace plansys2
{

UPFPlanSolver::UPFPlanSolver()
{
}

void
UPFPlanSolver::configure(rclcpp_lifecycle::LifecycleNode::SharedPtr & node, const std::string & id)
{
  if (!node->has_parameter(id + ".solver") || !node->get_parameter(id + ".solver", upf_solver_) ||
      upf_solver_ == "") {
    RCLCPP_INFO(node->get_logger(), "UPF solver not defined, using the default one");
    upf_solver_ = "popf";
  }

  RCLCPP_INFO(node->get_logger(), "UPF solver %s", upf_solver_.c_str());
  RCLCPP_INFO(node->get_logger(), "UPF configuration done");
}

std::optional<plansys2_msgs::msg::Plan>
UPFPlanSolver::getPlan(
  const std::string & domain, const std::string & problem,
  const std::string & node_namespace)
{
  std::string path = "/tmp";
  if (node_namespace != "") {
    //  This doesn't work as cxx flags must appear at end of link options, and I didn't
    //  find a way
    // std::experimental::filesystem::create_directories("/tmp/" + node_namespace);
    mkdir(("/tmp/" + node_namespace).c_str(), ACCESSPERMS);
    path = "/tmp/" + node_namespace;
  }

  plansys2_msgs::msg::Plan ret;

  std::ofstream domain_out(path + "/domain.pddl");
  domain_out << domain;
  domain_out.close();

  std::ofstream problem_out(path + "/problem.pddl");
  problem_out << problem;
  problem_out.close();

  std::string package_share_directory =
    ament_index_cpp::get_package_share_directory("plansys2_upf_plan_solver");

  system(
    (package_share_directory + "/plan.py " +
    upf_solver_ + " " +
    path + "/domain.pddl " +
    path + "/problem.pddl " +
    path + "/pddlplan").c_str()
  );

  std::string line;
  std::ifstream plan_file(path + "/pddlplan");

  if (plan_file.is_open()) {
    while (getline(plan_file, line)) {
      plansys2_msgs::msg::PlanItem item;
      size_t colon_pos = line.find(":");
      size_t colon_par = line.find(")");
      size_t colon_bra = line.find("[");

      std::string time = line.substr(0, colon_pos);
      std::string action = line.substr(colon_pos + 2, colon_par - colon_pos - 1);
      std::string duration = line.substr(colon_bra + 1);
      duration.pop_back();

      item.time = std::stof(time);
      item.action = action;
      item.duration = std::stof(duration);

      ret.items.push_back(item);
    }
    plan_file.close();
  }

  system(
    ("[ -f " + path + "/pddlplan ] && mv " +
    path + "/pddlplan " +
    path + "/pddlplan.last").c_str()
  );

  if (ret.items.empty()) {
    return {};
  } else {
    return ret;
  }
}

}  // namespace plansys2
