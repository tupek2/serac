# Copyright (c) 2019-2023, Lawrence Livermore National Security, LLC and
# other Serac Project Developers. See the top-level LICENSE file for
# details.
#
# SPDX-License-Identifier: (BSD-3-Clause)

set(state_headers
    finite_element_vector.hpp
    finite_element_state.hpp
    finite_element_dual.hpp
    state_manager.hpp
    )

set(state_sources
    finite_element_vector.cpp
    finite_element_state.cpp
    state_manager.cpp
    )

set(state_depends serac_infrastructure)

blt_add_library(
    NAME        serac_state
    HEADERS     ${state_headers}
    SOURCES     ${state_sources}
    DEPENDS_ON  ${state_depends}
    )

install(FILES ${state_headers} DESTINATION include/serac/physics/state )

install(TARGETS              serac_state
        EXPORT               serac-targets
        DESTINATION          lib
        )
