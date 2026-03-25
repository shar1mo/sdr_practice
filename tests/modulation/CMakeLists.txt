add_executable(
    test_modulation test_modulation.cpp
)

target_link_libraries(test_modulation     
        ${PYTHON_LIBRARIES}
        Python3::NumPy
    )
