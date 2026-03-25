include_directories("include/")

add_executable(
    end2end end2end.cpp ../../src/plots.cpp
)

target_link_libraries(end2end
        ${END2END_SOURCE_FILES}    
        ${PYTHON_LIBRARIES}
        Python3::NumPy
        mylib
    )
