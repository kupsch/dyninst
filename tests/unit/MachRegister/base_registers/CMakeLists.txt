include_guard(GLOBAL)

add_executable(basereg_x86 x86.cpp)
target_compile_options(basereg_x86 PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_x86 PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_x86 PRIVATE common)
add_test(NAME machregister_basereg_x86 COMMAND basereg_x86)
set_tests_properties(machregister_basereg_x86 PROPERTIES LABELS "unit")

add_executable(basereg_x86_64 x86_64.cpp)
target_compile_options(basereg_x86_64 PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_x86_64 PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_x86_64 PRIVATE common)
add_test(NAME machregister_basereg_x86_64 COMMAND basereg_x86_64)
set_tests_properties(machregister_basereg_x86_64 PROPERTIES LABELS "unit")

add_executable(basereg_aarch64 aarch64.cpp)
target_compile_options(basereg_aarch64 PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_aarch64 PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_aarch64 PRIVATE common)
add_test(NAME machregister_basereg_aarch64 COMMAND basereg_aarch64)
set_tests_properties(machregister_basereg_aarch64 PROPERTIES LABELS "unit")

add_executable(basereg_ppc32 ppc32.cpp)
target_compile_options(basereg_ppc32 PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_ppc32 PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_ppc32 PRIVATE common)
add_test(NAME machregister_basereg_ppc32 COMMAND basereg_ppc32)
set_tests_properties(machregister_basereg_ppc32 PROPERTIES LABELS "unit")

add_executable(basereg_ppc64 ppc64.cpp)
target_compile_options(basereg_ppc64 PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_ppc64 PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_ppc64 PRIVATE common)
add_test(NAME machregister_basereg_ppc64 COMMAND basereg_ppc64)
set_tests_properties(machregister_basereg_ppc64 PROPERTIES LABELS "unit")

add_executable(basereg_amdgpu_gfx908 amdgpu_gfx908.cpp)
target_compile_options(basereg_amdgpu_gfx908 PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_amdgpu_gfx908 PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_amdgpu_gfx908 PRIVATE common)
add_test(NAME machregister_basereg_amdgpu_gfx908 COMMAND basereg_amdgpu_gfx908)
set_tests_properties(machregister_basereg_amdgpu_gfx908 PROPERTIES LABELS "unit")

add_executable(basereg_amdgpu_gfx90a amdgpu_gfx90a.cpp)
target_compile_options(basereg_amdgpu_gfx90a PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_amdgpu_gfx90a PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_amdgpu_gfx90a PRIVATE common)
add_test(NAME machregister_basereg_amdgpu_gfx90a COMMAND basereg_amdgpu_gfx90a)
set_tests_properties(machregister_basereg_amdgpu_gfx90a PROPERTIES LABELS "unit")

add_executable(basereg_amdgpu_gfx940 amdgpu_gfx940.cpp)
target_compile_options(basereg_amdgpu_gfx940 PRIVATE ${SUPPORTED_CXX_WARNING_FLAGS})
target_include_directories(basereg_amdgpu_gfx940 PRIVATE ${UNIT_TESTS_INCLUDES})
target_link_libraries(basereg_amdgpu_gfx940 PRIVATE common)
add_test(NAME machregister_basereg_amdgpu_gfx940 COMMAND basereg_amdgpu_gfx940)
set_tests_properties(machregister_basereg_amdgpu_gfx940 PROPERTIES LABELS "unit")
