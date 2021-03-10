add_executable(testing viewer_test.cc cacher_test.cc)
find_package(GTest REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(testing GTest::GTest Qt5::Widgets lib)
