cmake_minimum_required(VERSION 3.20)
project(StateTap VERSION 0.1.0 LANGUAGES CXX)

add_library(statetap STATIC
    src/control_center.cpp
    src/transporter.cpp
)
add_library(StateTap::StateTap ALIAS statetap)
target_compile_features(statetap PUBLIC cxx_std_20)
target_include_directories(statetap PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(statetap_example examples/basic.cpp)
target_link_libraries(statetap_example PRIVATE StateTap::StateTap)

include(GNUInstallDirs)
install(TARGETS statetap EXPORT StateTapTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT StateTapTargets
    NAMESPACE StateTap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StateTap
)

