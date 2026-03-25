set(SOURCES bit_generation.cpp
            modulation.cpp
            pulse_shaping.cpp
            math_operations.cpp)


set(HEADERS bit_generation.h
            modulation.h
            pulse_shaping.h
            math_operations.h)



add_library(mylib ${SOURCES} ${HEADERS})
target_include_directories(mylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})