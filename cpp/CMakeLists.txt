cmake_minimum_required(VERSION 3.20)
project(StateTap VERSION 0.1.0 LANGUAGES CXX)

add_library(statetap STATIC
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

include(CTest)
if(BUILD_TESTING)
    add_executable(statetap_tap_test tests/tap_test.cpp)
    target_link_libraries(statetap_tap_test PRIVATE StateTap::StateTap)
    add_test(NAME tap_isolation COMMAND statetap_tap_test)

    add_executable(statetap_control_center_test tests/control_center_test.cpp)
    target_link_libraries(statetap_control_center_test PRIVATE StateTap::StateTap)
    add_test(NAME control_center_isolation COMMAND statetap_control_center_test)

    add_executable(statetap_tap_control_center_test tests/tap_control_center_test.cpp)
    target_link_libraries(statetap_tap_control_center_test PRIVATE StateTap::StateTap)
    add_test(NAME tap_control_center COMMAND statetap_tap_control_center_test)
endif()

include(GNUInstallDirs)
install(TARGETS statetap EXPORT StateTapTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT StateTapTargets
    NAMESPACE StateTap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StateTap
)

