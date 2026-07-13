find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(morfupdate_demo main.cpp)
target_link_libraries(morfupdate_demo PRIVATE
    morfUpdate::morfUpdate Qt6::Core)
