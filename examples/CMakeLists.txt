# Copyright (c) 2019-2023, Lawrence Livermore National Security, LLC and
# other Serac Project Developers. See the top-level LICENSE file for
# details.
#
# SPDX-License-Identifier: (BSD-3-Clause)

configure_file(
    using-with-cmake/host-config.cmake.in
    ${CMAKE_BINARY_DIR}/examples/using-with-cmake/host-config.cmake)


install(
    FILES
        using-with-cmake/CMakeLists.txt
        using-with-cmake/serac_example.cpp
        ${CMAKE_BINARY_DIR}/examples/using-with-cmake/host-config.cmake
    DESTINATION
        examples/serac/using-with-cmake
)

# Add the simple conduction examples
add_subdirectory(simple_conduction)

install(
    FILES
        simple_conduction/without_input_file.cpp
    DESTINATION
        examples/serac/simple_conduction
)
