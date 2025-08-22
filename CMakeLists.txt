cmake_minimum_required (VERSION 3.24)

project ("preproq")

cmake_policy(SET CMP0079 NEW)

set(CMAKE_CXX_STANDARD 20)

if(DEBUG_ENABLE_ADDRESS_SANITIZER)
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined")
    message("Building with enabled address sanitizer")
endif()

set(SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/qcir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/preproq.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/writer.cpp  
)

#Might be changed to STATIC on demand
add_library(libpreproq OBJECT ${SRCS})
target_include_directories(libpreproq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable (preproq "main.cpp")
target_link_libraries(preproq PUBLIC libpreproq)

add_executable (preproqTests "tests.cpp")
target_link_libraries(preproqTests PUBLIC libpreproq)
add_subdirectory(thirdparty/catch2)
target_link_libraries(preproqTests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)

target_compile_definitions(preproqTests PUBLIC PREPROQ_TESTING=1)

enable_testing()
