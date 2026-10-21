# One test executable per manager.
function(snapper_add_test name)
  qt_add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE snapper_core Qt6::Test)
  target_compile_definitions(${name} PRIVATE
    SNAPPER_THEME_FILE="${PROJECT_SOURCE_DIR}/theme/theme.json")
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

snapper_add_test(tst_thememanager)
snapper_add_test(tst_commandregistry)
snapper_add_test(tst_settingsmanager)
