add_executable(morfupdate-agent main.cpp)
target_link_libraries(morfupdate-agent PRIVATE morfUpdate::morfUpdate Qt6::Core Qt6::Network)
set_target_properties(morfupdate-agent PROPERTIES
    OUTPUT_NAME morfupdate
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/service"
)

add_executable(morfupdate-helper privileged-helper.cpp)
target_link_libraries(morfupdate-helper PRIVATE Qt6::Core)
set_target_properties(morfupdate-helper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/service"
)

if(MSVC)
    target_compile_options(morfupdate-agent PRIVATE /utf-8)
    target_compile_options(morfupdate-helper PRIVATE /utf-8)
endif()

install(TARGETS morfupdate-agent morfupdate-helper RUNTIME DESTINATION bin)
