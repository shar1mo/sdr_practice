add_executable(
    test_pulse_shapes test_pulse_shapes.cpp
)

target_link_libraries(test_pulse_shapes     
        ${PYTHON_LIBRARIES}
        Python3::NumPy
        mylib
    )
