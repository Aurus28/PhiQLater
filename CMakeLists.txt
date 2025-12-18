cmake_minimum_required(VERSION 3.16)
project(PhiQLater VERSION 0.1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(GMP REQUIRED gmp)

add_executable(PhiQLater main.c)


target_include_directories(PhiQLater PRIVATE ${GTK4_INCLUDE_DIRS} ${GMP_INCLUDE_DIRS})
target_link_libraries(PhiQLater  PRIVATE ${GTK4_LIBRARIES} ${GMP_LIBRARIES} m)
target_compile_options(PhiQLater PRIVATE ${GTK4_CFLAGS_OTHER})


target_link_options(PhiQLater PRIVATE -Wl,--export-dynamic)
