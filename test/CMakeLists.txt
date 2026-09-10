# Tests du contrat morfUpdate (etats + reconciliation self-update).
add_executable(morfupdate-contract-test contract_test.cpp)
target_link_libraries(morfupdate-contract-test PRIVATE morfUpdate::morfUpdate Qt6::Core)
if(MSVC)
    target_compile_options(morfupdate-contract-test PRIVATE /utf-8)
endif()
add_test(NAME morfupdate-contract COMMAND morfupdate-contract-test)
