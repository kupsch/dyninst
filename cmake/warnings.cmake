# REQUESTED_WARNING_FLAGS is a list of warning flags for C and C++ programs
# to enable if supported by the compiler.  The values do not include the
# the initial '-'

list(APPEND REQUESTED_WARNING_FLAGS
        Wall
        Wextra
        Wpedantic

	Wctor-dtor-privacy
	Wenum-conversions
	Wmissing-braces
	Wmultichar
        Walloca
        Wcast-align
        Wcast-qual
        Wcomma-subscript
        Wdeprecated-copy-dtor
        Wdouble-promotion
        Wduplicated-branches
        Wduplicated-cond
        Wextra-semi
        Wfloat-equal
        Wformat-overflow=2
        Wformat-signedness
        Wformat=2
        Wframe-larger-than=131072
        Wjump-misses-init
        Wlogical-op
        Wmismatched-tags
        Wnoexcept
        Wnon-virtual-dtor
        Woverloaded-virtual
        Wpointer-arith
        Wrange-loop-construct
        Wrestrict
        Wshadow
        Wsuggest-attribute=format
        Wsuggest-attribute=malloc
        Wuninitialized
        Wvla
        Wvolatile
        Wwrite-strings
    )

#list(APPEND REQUESTED_WARNING_FLAGS Werror)

#list(APPEND REQUESTED_WARNING_FLAGS Wredundant-tags)
#list(APPEND REQUESTED_WARNING_FLAGS Wnull-dereference)
#list(APPEND REQUESTED_WARNING_FLAGS Wconversion)
#list(APPEND REQUESTED_WARNING_FLAGS Wzero-as-null-pointer-constant)
#list(APPEND REQUESTED_WARNING_FLAGS Wuseless-cast)
#list(APPEND REQUESTED_WARNING_FLAGS Wsuggest-override)
#list(APPEND REQUESTED_WARNING_FLAGS Wsuggest-final-types)
#list(APPEND REQUESTED_WARNING_FLAGS Wsuggest-final-methods)
#list(APPEND REQUESTED_WARNING_FLAGS Wsign-promo)
#list(APPEND REQUESTED_WARNING_FLAGS Wold-style-cast)
#list(APPEND REQUESTED_WARNING_FLAGS Walloc-zero)
#list(APPEND REQUESTED_WARNING_FLAGS Wstrict-null-sentinal)

if (CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
  include(CheckCCompilerFlag)
  foreach (f IN LISTS REQUESTED_WARNING_FLAGS)
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" v "HAS_C_FLAG_${f}")
    set(CMAKE_REQUIRED_FLAGS "-${f}")
    check_c_source_compiles("int main(){return 0;}" "${v}" FAIL_REGEX "warning: *command[- ]line option")
    # Previous two lines are equivalent to below, but also catches
    # a 0 exit status with a warning message output:
    #    check_c_compiler_flag("-${f}" "${v}")
    if (${v})
        string(APPEND SUPPORTED_C_WARNING_FLAGS " -${f}")
    endif()
  endforeach()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  include(CheckCXXCompilerFlag)
  foreach (f IN LISTS REQUESTED_WARNING_FLAGS)
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" v "HAS_CPP_FLAG_${f}")
    set(CMAKE_REQUIRED_FLAGS "-${f}")
    check_cxx_source_compiles("int main(){return 0;}" "${v}" FAIL_REGEX "warning: *command[- ]line option")
    if (${v})
        string(APPEND SUPPORTED_CXX_WARNING_FLAGS " -${f}")
    endif()
  endforeach()
endif()

unset(CMAKE_REQUIRED_FLAGS)

if (MSVC)
  message(STATUS "TODO: Set up custom warning flags for MSVC")
  string(APPEND CMAKE_C_FLAGS "/wd4251 /wd4091 /wd4503")
  string(APPEND CMAKE_CXX_FLAGS "/wd4251 /wd4091 /wd4503")
endif()

message(STATUS "Using C warning flags: ${SUPPORTED_C_WARNING_FLAGS}")
message(STATUS "Using CPP warning flags: ${SUPPORTED_CXX_WARNING_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SUPPORTED_C_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SUPPORTED_CXX_WARNING_FLAGS}")
