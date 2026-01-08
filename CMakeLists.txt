cmake_minimum_required(VERSION 3.16)
project(PhiQLater VERSION 0.1.0 LANGUAGES C)

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(GMP REQUIRED gmp)
pkg_check_modules(MPFR REQUIRED mpfr)


set(RESOURCE resources/phiqlater.gresource.xml)
set(OUTPUT_C gresource.c)

add_custom_command(
        OUTPUT ${OUTPUT_C}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${GLIB_COMPILE_RESOURCES}
        ARGS --generate-source --target=${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_C} ${RESOURCE}
        VERBATIM
        MAIN_DEPENDENCY ${RESOURCE}
        DEPENDS
            resources/phiqlater.ui
            resources/calc_row.ui
)

add_custom_target(
        ui-resource
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_C}
)


add_executable(PhiQLater main.c ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_C})
set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_C}
        PROPERTIES GENERATED TRUE
)

add_dependencies(${PROJECT_NAME} ui-resource)

target_include_directories(PhiQLater PRIVATE ${GTK4_INCLUDE_DIRS} ${GMP_INCLUDE_DIRS} ${MPFR_INCLUDE_DIRS})
target_link_libraries(PhiQLater  PRIVATE ${GTK4_LIBRARIES} ${GMP_LIBRARIES} ${MPFR_LIBRARIES} m)
target_compile_options(PhiQLater PRIVATE ${GTK4_CFLAGS_OTHER})


target_link_options(PhiQLater PRIVATE -Wl,--export-dynamic)

install(TARGETS PhiQLater
        RUNTIME DESTINATION bin)