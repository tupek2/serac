// Copyright (c) 2019-2023, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include <fstream>

#include "axom/slic/core/SimpleLogger.hpp"
#include <gtest/gtest.h>
#include "mfem.hpp"

#include "serac/serac_config.hpp"
#include "serac/mesh/mesh_utils.hpp"
#include "serac/physics/solid_mechanics.hpp"
#include "serac/physics/materials/solid_material.hpp"
#include "serac/physics/materials/parameterized_solid_material.hpp"
#include "serac/physics/state/state_manager.hpp"

namespace serac {

TEST(SolidMechanics, FiniteDifferenceParameter)
{
  MPI_Barrier(MPI_COMM_WORLD);

  int serial_refinement   = 1;
  int parallel_refinement = 0;

  // Create DataStore
  axom::sidre::DataStore datastore;
  serac::StateManager::initialize(datastore, "solid_functional_parameterized_sensitivities");

  // Construct the appropriate dimension mesh and give it to the data store
  std::string filename = SERAC_REPO_DIR "/data/meshes/patch2D_tris_and_quads.mesh";

  auto mesh = mesh::refineAndDistribute(buildMeshFromFile(filename), serial_refinement, parallel_refinement);
  serac::StateManager::setMesh(std::move(mesh));

  constexpr int p   = 1;
  constexpr int dim = 2;

  // Construct and initialized the user-defined moduli to be used as a differentiable parameter in
  // the solid physics module.
  FiniteElementState user_defined_shear_modulus(
      StateManager::newState(FiniteElementState::Options{.order = 1, .name = "parameterized_shear"}));

  double shear_modulus_value = 1.0;

  user_defined_shear_modulus = shear_modulus_value;

  FiniteElementState user_defined_bulk_modulus(
      StateManager::newState(FiniteElementState::Options{.order = 1, .name = "parameterized_bulk"}));

  double bulk_modulus_value = 1.0;

  user_defined_bulk_modulus = bulk_modulus_value;

  // Construct a functional-based solid solver

  auto lin_options          = solid_mechanics::default_linear_options;
  lin_options.linear_solver = LinearSolver::SuperLU;

  SolidMechanics<p, dim, Parameters<H1<1>, H1<1>>> solid_solver(solid_mechanics::default_nonlinear_options, lin_options,
                                                                solid_mechanics::default_quasistatic_options,
                                                                GeometricNonlinearities::On, "solid_functional");

  solid_solver.setParameter(0, user_defined_bulk_modulus);
  solid_solver.setParameter(1, user_defined_shear_modulus);

  // We must know the index of the parameter finite element state in our parameter pack to take sensitivities.
  // As we only have one parameter in this example, the index is zero.
  constexpr int bulk_parameter_index = 0;

  solid_mechanics::ParameterizedNeoHookeanSolid<dim> mat{1.0, 0.0, 0.0};
  solid_solver.setMaterial(DependsOn<0, 1>{}, mat);

  // Define the function for the initial displacement and boundary condition
  auto bc = [](const mfem::Vector&, mfem::Vector& bc_vec) -> void { bc_vec = 0.0; };

  // Define a boundary attribute set and specify initial / boundary conditions
  std::set<int> ess_bdr = {1};
  solid_solver.setDisplacementBCs(ess_bdr, bc);
  solid_solver.setDisplacement(bc);

  tensor<double, dim> constant_force;

  constant_force[0] = 0.0;
  constant_force[1] = 1.0e-3;

  if (dim == 3) {
    constant_force[2] = 0.0;
  }

  solid_mechanics::ConstantBodyForce<dim> force{constant_force};
  solid_solver.addBodyForce(force);

  // Finalize the data structures
  solid_solver.completeSetup();

  // Perform the quasi-static solve
  double dt = 1.0;
  solid_solver.advanceTimestep(dt);

  // Output the sidre-based plot files
  solid_solver.outputState();

  // Make up an adjoint load which can also be viewed as a
  // sensitivity of some qoi with respect to displacement
  mfem::ParLinearForm adjoint_load_form(&solid_solver.displacement().space());
  adjoint_load_form = 1.0;

  // Construct a dummy adjoint load (this would come from a QOI downstream).
  // This adjoint load is equivalent to a discrete L1 norm on the displacement.
  serac::FiniteElementDual              adjoint_load(solid_solver.displacement().space(), "adjoint_load");
  std::unique_ptr<mfem::HypreParVector> assembled_vector(adjoint_load_form.ParallelAssemble());
  adjoint_load = *assembled_vector;

  // Solve the adjoint problem
  solid_solver.solveAdjoint({{"displacement", adjoint_load}});

  // Compute the sensitivity (d QOI/ d state * d state/d parameter) given the current adjoint solution
  [[maybe_unused]] auto& sensitivity = solid_solver.computeSensitivity(bulk_parameter_index);

  // Perform finite difference on each bulk modulus value
  // to check if computed qoi sensitivity is consistent
  // with finite difference on the displacement
  double eps = 1.0e-5;
  for (int i = 0; i < user_defined_bulk_modulus.gridFunction().Size(); ++i) {
    // Perturb the bulk modulus
    user_defined_bulk_modulus(i) = bulk_modulus_value + eps;
    solid_solver.setDisplacement(bc);

    solid_solver.advanceTimestep(dt);
    mfem::ParGridFunction displacement_plus = solid_solver.displacement().gridFunction();

    user_defined_bulk_modulus(i) = bulk_modulus_value - eps;

    solid_solver.setDisplacement(bc);
    solid_solver.advanceTimestep(dt);
    mfem::ParGridFunction displacement_minus = solid_solver.displacement().gridFunction();

    // Reset to the original bulk modulus value
    user_defined_bulk_modulus(i) = bulk_modulus_value;

    // Finite difference to compute sensitivity of displacement with respect to bulk modulus
    mfem::ParGridFunction ddisp_dbulk(&solid_solver.displacement().space());
    for (int i2 = 0; i2 < displacement_plus.Size(); ++i2) {
      ddisp_dbulk(i2) = (displacement_plus(i2) - displacement_minus(i2)) / (2.0 * eps);
    }

    // Compute numerical value of sensitivity of qoi with respect to bulk modulus
    // by taking the inner product between adjoint load and displacement sensitivity
    double dqoi_dbulk = adjoint_load_form(ddisp_dbulk);

    // See if these are similar
    SLIC_INFO(axom::fmt::format("dqoi_dbulk: {}", dqoi_dbulk));
    SLIC_INFO(axom::fmt::format("sensitivity: {}", sensitivity(i)));
    double relative_error = (sensitivity(i) - dqoi_dbulk) / std::max(dqoi_dbulk, 1.0e-2);
    EXPECT_NEAR(relative_error, 0.0, 5.0e-4);
  }
}

/**
 * @brief Specify the kind of loading to apply
 */
enum class LoadingType
{
  BodyForce,
  Pressure,
  Traction
};

/**
 * @brief A driver for a shape sensitivity test
 *
 * This performs a finite difference check for the sensitivities of the shape displacements
 * for various loading types. It can currently only run in serial.
 *
 * @param load The type of loading to apply to the problem
 */
void finite_difference_shape_test(LoadingType load)
{
  MPI_Barrier(MPI_COMM_WORLD);

  int serial_refinement   = 1;
  int parallel_refinement = 0;

  // Create DataStore
  axom::sidre::DataStore datastore;
  serac::StateManager::initialize(datastore, "solid_functional_parameterized_shape_sensitivities");

  // Construct the appropriate dimension mesh and give it to the data store
  std::string filename = SERAC_REPO_DIR "/data/meshes/patch2D_tris.mesh";

  auto mesh = mesh::refineAndDistribute(buildMeshFromFile(filename), serial_refinement, parallel_refinement);
  serac::StateManager::setMesh(std::move(mesh));

  constexpr int p   = 1;
  constexpr int dim = 2;

  // Define a boundary attribute set
  std::set<int> ess_bdr = {1};

  double shape_displacement_value = 1.0;

  // The nonlinear solver must have tight tolerances to ensure at least one Newton step occurs
  serac::NonlinearSolverOptions nonlin_options{
      .relative_tol = 1.0e-8, .absolute_tol = 1.0e-14, .max_iterations = 10, .print_level = 1};

  // Construct a functional-based solid solver
  SolidMechanics<p, dim> solid_solver(nonlin_options, solid_mechanics::direct_linear_options,
                                      solid_mechanics::default_quasistatic_options, GeometricNonlinearities::On,
                                      "solid_functional");

  solid_mechanics::NeoHookean mat{1.0, 1.0, 1.0};
  solid_solver.setMaterial(mat);

  FiniteElementState shape_displacement(solid_solver.shapeDisplacement());

  shape_displacement = shape_displacement_value;
  solid_solver.setShapeDisplacement(shape_displacement);

  // Define the function for the initial displacement and boundary condition
  auto bc = [](const mfem::Vector&, mfem::Vector& bc_vec) -> void { bc_vec = 0.0; };

  // Set the initial displacement and boundary condition
  solid_solver.setDisplacementBCs(ess_bdr, bc);
  solid_solver.setDisplacement(bc);

  if (load == LoadingType::BodyForce) {
    tensor<double, dim> constant_force;

    constant_force[0] = 0.0;
    constant_force[1] = 1.0e-1;

    if (dim == 3) {
      constant_force[2] = 0.0;
    }

    solid_mechanics::ConstantBodyForce<dim> force{constant_force};
    solid_solver.addBodyForce(force);
  } else if (load == LoadingType::Pressure) {
    solid_solver.setPressure([](auto& X, double) {
      if (X[1] > 0.99) {
        return 0.1;
      }
      return 0.0;
    });
  } else if (load == LoadingType::Traction) {
    solid_solver.setTraction([](auto& X, auto, double) {
      auto traction = 0.0 * X;
      if (X[1] > 0.99) {
        traction[0] = 1.0e-2;
        traction[1] = 1.0e-2;
      }
      return traction;
    });
  }

  // Finalize the data structures
  solid_solver.completeSetup();

  // Perform the quasi-static solve
  double dt = 1.0;
  solid_solver.advanceTimestep(dt);

  // Output the sidre-based plot files
  solid_solver.outputState();

  // Make up an adjoint load which can also be viewed as a
  // sensitivity of some qoi with respect to displacement
  mfem::ParLinearForm adjoint_load_form(&solid_solver.displacement().space());
  adjoint_load_form = 1.0;

  // Construct a dummy adjoint load (this would come from a QOI downstream).
  // This adjoint load is equivalent to a discrete L1 norm on the displacement.
  serac::FiniteElementDual              adjoint_load(solid_solver.displacement().space(), "adjoint_load");
  std::unique_ptr<mfem::HypreParVector> assembled_vector(adjoint_load_form.ParallelAssemble());
  adjoint_load = *assembled_vector;

  // Solve the adjoint problem
  solid_solver.solveAdjoint({{"displacement", adjoint_load}});

  // Compute the sensitivity (d QOI/ d state * d state/d parameter) given the current adjoint solution
  [[maybe_unused]] auto& sensitivity = solid_solver.computeShapeSensitivity();

  // Perform finite difference on each shape velocity value
  // to check if computed qoi sensitivity is consistent
  // with finite difference on the displacement
  double eps = 1.0e-6;
  for (int i = 0; i < shape_displacement.Size(); ++i) {
    // Perturb the shape field
    shape_displacement(i) = shape_displacement_value + eps;
    solid_solver.setShapeDisplacement(shape_displacement);

    solid_solver.advanceTimestep(dt);
    mfem::ParGridFunction displacement_plus = solid_solver.displacement().gridFunction();

    shape_displacement(i) = shape_displacement_value - eps;
    solid_solver.setShapeDisplacement(shape_displacement);

    solid_solver.advanceTimestep(dt);
    mfem::ParGridFunction displacement_minus = solid_solver.displacement().gridFunction();

    // Reset to the original bulk modulus value
    shape_displacement(i) = shape_displacement_value;
    solid_solver.setShapeDisplacement(shape_displacement);

    // Finite difference to compute sensitivity of displacement with respect to bulk modulus
    mfem::ParGridFunction ddisp_dshape(&solid_solver.displacement().space());
    for (int i2 = 0; i2 < displacement_plus.Size(); ++i2) {
      ddisp_dshape(i2) = (displacement_plus(i2) - displacement_minus(i2)) / (2.0 * eps);
    }

    // Compute numerical value of sensitivity of qoi with respect to bulk modulus
    // by taking the inner product between adjoint load and displacement sensitivity
    double dqoi_dshape = adjoint_load_form(ddisp_dshape);

    // See if these are similar
    SLIC_INFO(axom::fmt::format("dqoi_dshape: {}", dqoi_dshape));
    SLIC_INFO(axom::fmt::format("sensitivity: {}", sensitivity(i)));
    EXPECT_NEAR((sensitivity(i) - dqoi_dshape) / std::max(dqoi_dshape, 1.0e-3), 0.0, 1.0e-4);
  }
}

TEST(SolidMechanicsShape, BodyForce) { finite_difference_shape_test(LoadingType::BodyForce); }
TEST(SolidMechanicsShape, Pressure) { finite_difference_shape_test(LoadingType::Pressure); }
TEST(SolidMechanicsShape, Traction) { finite_difference_shape_test(LoadingType::Traction); }

}  // namespace serac

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  MPI_Init(&argc, &argv);

  axom::slic::SimpleLogger logger;

  int result = RUN_ALL_TESTS();
  MPI_Finalize();

  return result;
}
