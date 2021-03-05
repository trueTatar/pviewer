add_executable(app main.cpp)
find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
target_compile_features(app PRIVATE cxx_std_20)
target_link_libraries(app PRIVATE lib PRIVATE Qt5::Widgets PRIVATE Qt5::Network)