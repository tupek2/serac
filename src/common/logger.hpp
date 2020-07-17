// Copyright (c) 2019-2020, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef SERAC_LOGGER
#define SERAC_LOGGER

#include "axom/slic.hpp"

#include "mpi.h"
#include "fmt/fmt.hpp"

namespace serac {
  // Gracefully exit program
  void ExitGracefully(bool error=false);

// Logger functionality
namespace logger {
  bool Initialize(MPI_Comm comm);
  void Finalize();
  void Flush();

}  // namespace logger
}  // namespace serac

// Utility SLIC macros

#define SLIC_ERROR_MASTER(rank,msg)   SLIC_ERROR_IF(rank==0, msg)
#define SLIC_WARNING_MASTER(rank,msg) SLIC_WARNING_IF(rank==0, msg)
#define SLIC_INFO_MASTER(rank,msg)    SLIC_INFO_IF(rank==0, msg)
#define SLIC_DEBUG_MASTER(rank,msg)   SLIC_DEBUG_IF(rank==0, msg)

#endif