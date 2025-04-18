# Copyright (c) 2019-2023, Lawrence Livermore National Security, LLC and
# other Serac Project Developers. See the top-level LICENSE file for
# details.
#
# SPDX-License-Identifier: (BSD-3-Clause)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

# This file contains metadata that often changes (ie. git sha).
# Hide these changes in the source file so we do not force
# a full rebuild.
serac_configure_file(
  git_sha.hpp.in
  ${PROJECT_BINARY_DIR}/include/serac/infrastructure/git_sha.hpp
 )

set(infrastructure_headers
    about.hpp
    accelerator.hpp
    cli.hpp
    debug_print.hpp
    ${PROJECT_BINARY_DIR}/include/serac/infrastructure/git_sha.hpp
    initialize.hpp
    input.hpp
    logger.hpp
    mpi_fstream.hpp
    output.hpp
    profiling.hpp
    terminator.hpp
    variant.hpp
    )

set(infrastructure_sources
    about.cpp
    accelerator.cpp
    cli.cpp
    initialize.cpp
    input.cpp
    logger.cpp
    mpi_fstream.cpp
    output.cpp
    profiling.cpp
    terminator.cpp
    )

set(infrastructure_depends axom axom::fmt axom::cli11 mfem)
blt_list_append(TO infrastructure_depends ELEMENTS tribol IF TRIBOL_FOUND)
blt_list_append(TO infrastructure_depends ELEMENTS caliper adiak::adiak IF SERAC_ENABLE_PROFILING)
blt_list_append(TO infrastructure_depends ELEMENTS cuda IF ENABLE_CUDA)
list(APPEND infrastructure_depends mpi)

blt_add_library(
    NAME        serac_infrastructure
    HEADERS     ${infrastructure_headers}
    SOURCES     ${infrastructure_sources}
    DEPENDS_ON  ${infrastructure_depends}
    )

# Set file back to C++ due to nvcc compiler error
set_source_files_properties(input.cpp PROPERTIES LANGUAGE CXX)

target_include_directories(serac_infrastructure PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
    )

install(FILES ${infrastructure_headers} DESTINATION include/serac/infrastructure )

install(TARGETS              serac_infrastructure
        EXPORT               serac-targets
        DESTINATION          lib
        )
