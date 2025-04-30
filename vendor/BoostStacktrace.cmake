find_package(Boost CONFIG REQUIRED stacktrace_basic stacktrace_addr2line)

if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    add_definitions(-DBOOST_STACKTRACE_USE_ADDR2LINE)
else()
    message(FATAL_ERROR "Boost.Stacktrace library not found!")
endif()
