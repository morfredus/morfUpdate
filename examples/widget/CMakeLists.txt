find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(morfupdate_widget_demo main.cpp)
target_link_libraries(morfupdate_widget_demo PRIVATE
    morfUpdate::Widgets Qt6::Widgets)
