// Copyright (c) 2019-2020, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "base_solver.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "common/logger.hpp"
#include "common/serac_types.hpp"
#include "fmt/fmt.hpp"

BaseSolver::BaseSolver(MPI_Comm comm) : m_comm(comm), m_output_type(OutputType::VisIt), m_time(0.0), m_cycle(0)
{
  MPI_Comm_rank(m_comm, &m_rank);
  BaseSolver::SetTimestepper(TimestepMethod::ForwardEuler);
}

BaseSolver::BaseSolver(MPI_Comm comm, int n) : BaseSolver(comm)
{
  m_state.resize(n);

  std::generate(m_state.begin(), m_state.end(), std::make_shared<FiniteElementState>);

  m_gf_initialized.assign(n, false);
}

void BaseSolver::SetEssentialBCs(const std::set<int> &                 ess_bdr,
                                 std::shared_ptr<mfem::VectorCoefficient> ess_bdr_vec_coef,
                                 const mfem::ParFiniteElementSpace &fes, const int component)
{
  auto bc = std::make_shared<BoundaryCondition>();
  bc->vec_coef  = ess_bdr_vec_coef;

  RegisterEssentialBC(bc, ess_bdr, fes, component);
}

void BaseSolver::SetTrueDofs(const mfem::Array<int> &                 true_dofs,
                             const std::shared_ptr<mfem::VectorCoefficient> ess_bdr_vec_coef)
{
  auto bc = std::make_shared<BoundaryCondition>();

  bc->markers.SetSize(0);

  bc->true_dofs = true_dofs;

  bc->vec_coef = ess_bdr_vec_coef;

  m_ess_bdr.push_back(bc);
}

void BaseSolver::SetNaturalBCs(const std::set<int> &                 nat_bdr,
                               std::shared_ptr<mfem::VectorCoefficient> nat_bdr_vec_coef, const int component)
{
 auto bc = std::make_shared<BoundaryCondition>();
  bc->vec_coef  = nat_bdr_vec_coef;

  RegisterNaturalBC(bc, nat_bdr, component);
}

void BaseSolver::SetEssentialBCs(const std::set<int> &ess_bdr, std::shared_ptr<mfem::Coefficient> ess_bdr_coef,
                                 const mfem::ParFiniteElementSpace &fes, const int component)
{
  auto bc = std::make_shared<BoundaryCondition>();
  bc->scalar_coef  = ess_bdr_coef;

  RegisterEssentialBC(bc, ess_bdr, fes, component);
}

void BaseSolver::SetTrueDofs(const mfem::Array<int> &true_dofs, std::shared_ptr<mfem::Coefficient> ess_bdr_coef)
{
  auto bc = std::make_shared<BoundaryCondition>();

  bc->markers.SetSize(0);

  bc->true_dofs = true_dofs;

  bc->scalar_coef = ess_bdr_coef;

  m_ess_bdr.push_back(bc);
}

void BaseSolver::SetNaturalBCs(const std::set<int> &nat_bdr, std::shared_ptr<mfem::Coefficient> nat_bdr_coef,
                               const int component)
{
  auto bc = std::make_shared<BoundaryCondition>();
  bc->scalar_coef  = nat_bdr_coef;

  RegisterNaturalBC(bc, nat_bdr, component);
}

void BaseSolver::SetState(const std::vector<std::shared_ptr<mfem::Coefficient> > &state_coef)
{
  SLIC_ASSERT_MSG(state_coef.size() == m_state.size(),
                  "State and coefficient bundles not the same size.");

  for (unsigned int i = 0; i < state_coef.size(); ++i) {
    m_state[i]->gf->ProjectCoefficient(*state_coef[i]);
  }
}

void BaseSolver::SetState(const std::vector<std::shared_ptr<mfem::VectorCoefficient> > &state_vec_coef)
{
  SLIC_ASSERT_MSG(state_vec_coef.size() == m_state.size(),
                  "State and coefficient bundles not the same size.");

  for (unsigned int i = 0; i < state_vec_coef.size(); ++i) {
    m_state[i]->gf->ProjectCoefficient(*state_vec_coef[i]);
  }
}

void BaseSolver::SetState(const std::vector<std::shared_ptr<FiniteElementState> > &state)
{
  SLIC_ASSERT_MSG(state.size() > 0, "State vector array of size 0.");
  m_state = state;
}

std::vector<std::shared_ptr<FiniteElementState> > BaseSolver::GetState() const { return m_state; }

void BaseSolver::SetTimestepper(const TimestepMethod timestepper)
{
  m_timestepper = timestepper;

  switch (m_timestepper) {
    case TimestepMethod::QuasiStatic:
      break;
    case TimestepMethod::BackwardEuler:
      m_ode_solver = std::make_unique<mfem::BackwardEulerSolver>();
      break;
    case TimestepMethod::SDIRK33:
      m_ode_solver = std::make_unique<mfem::SDIRK33Solver>();
      break;
    case TimestepMethod::ForwardEuler:
      m_ode_solver = std::make_unique<mfem::ForwardEulerSolver>();
      break;
    case TimestepMethod::RK2:
      m_ode_solver = std::make_unique<mfem::RK2Solver>(0.5);
      break;
    case TimestepMethod::RK3SSP:
      m_ode_solver = std::make_unique<mfem::RK3SSPSolver>();
      break;
    case TimestepMethod::RK4:
      m_ode_solver = std::make_unique<mfem::RK4Solver>();
      break;
    case TimestepMethod::GeneralizedAlpha:
      m_ode_solver = std::make_unique<mfem::GeneralizedAlphaSolver>(0.5);
      break;
    case TimestepMethod::ImplicitMidpoint:
      m_ode_solver = std::make_unique<mfem::ImplicitMidpointSolver>();
      break;
    case TimestepMethod::SDIRK23:
      m_ode_solver = std::make_unique<mfem::SDIRK23Solver>();
      break;
    case TimestepMethod::SDIRK34:
      m_ode_solver = std::make_unique<mfem::SDIRK34Solver>();
      break;
    default:
      SLIC_ERROR_MASTER(m_rank, "Timestep method not recognized!");
      serac::ExitGracefully(true);
  }
}

void BaseSolver::SetTime(const double time) { m_time = time; }

double BaseSolver::GetTime() const { return m_time; }

int BaseSolver::GetCycle() const { return m_cycle; }

void BaseSolver::InitializeOutput(const OutputType output_type, const std::string &root_name)
{
  m_root_name = root_name;

  m_output_type = output_type;

  switch (m_output_type) {
    case OutputType::VisIt: {
      m_visit_dc = std::make_unique<mfem::VisItDataCollection>(m_root_name, m_state.front()->mesh.get());
      for (const auto &state : m_state) {
        m_visit_dc->RegisterField(state->name, state->gf.get());
      }
      break;
    }

    case OutputType::GLVis: {
      break;
    }

    default:
      SLIC_ERROR_MASTER(m_rank, "OutputType not recognized!");
      serac::ExitGracefully(true);
  }
}

void BaseSolver::OutputState() const
{
  switch (m_output_type) {
    case OutputType::VisIt: {
      m_visit_dc->SetCycle(m_cycle);
      m_visit_dc->SetTime(m_time);
      m_visit_dc->Save();
      break;
    }

    case OutputType::GLVis: {
      std::string   mesh_name = fmt::format("{0}-mesh.{1:0>6}.{2:0>6}", m_root_name, m_cycle, m_rank);
      std::ofstream omesh(mesh_name.c_str());
      omesh.precision(8);
      m_state.front()->mesh->Print(omesh);

      for (auto &state : m_state) {
        std::string   sol_name = fmt::format("{0}-{1}.{2:0>6}.{3:0>6}", m_root_name, state->name, m_cycle, m_rank);
        std::ofstream osol(sol_name.c_str());
        osol.precision(8);
        state->gf->Save(osol);
      }
      break;
    }

    default:
      SLIC_ERROR_MASTER(m_rank, "OutputType not recognized!");
      serac::ExitGracefully(true);
  }
}

void BaseSolver::RegisterEssentialBC(std::shared_ptr<BoundaryCondition> bc, const std::set<int> &ess_bdr, 
                                     mfem::ParFiniteElementSpace fes, const int component)
{
  bc->markers.SetSize(m_state.front()->mesh->bdr_attributes.Max());
  bc->markers = 0;

  for (const int attr : ess_bdr) {
    SLIC_ASSERT_MSG(attr <= bc->markers.Size(), "Attribute specified larger than what is found in the mesh.");
    bc->markers[attr-1] = 1;
    if (std::any_of(m_ess_bdr.begin(), m_ess_bdr.end(), [attr](const auto& existing_bc) {
          return existing_bc->markers[attr-1] == 1;
        })) {
      SLIC_WARNING("Multiple definition of essential boundary! Using first definition given.");
      bc->markers[attr-1] = 0;
    }
  }

  bc->component = component;

  // This function can and should be marked const in MFEM
  // TODO: Raise an issue against MFEM
  // Just make a copy of the PFES for now to keep everything else const correct
  fes.GetEssentialTrueDofs(bc->markers, bc->true_dofs, component);

  m_ess_bdr.push_back(bc);
}

void BaseSolver::RegisterNaturalBC(std::shared_ptr<BoundaryCondition> bc, const std::set<int> &nat_bdr, 
                                   const int component)
{
  bc->markers.SetSize(m_state.front()->mesh->bdr_attributes.Max());
  bc->markers = 0;

  for (const int attr : nat_bdr) {  
    SLIC_ASSERT_MSG(attr <= bc->markers.Size(), "Attribute specified larger than what is found in the mesh.");
    bc->markers[attr-1] = 1;
  }
  bc->component   = component;

  m_nat_bdr.push_back(bc);
}