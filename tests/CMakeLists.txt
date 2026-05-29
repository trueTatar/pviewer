add_executable(testing viewer_test.cc cacher_test.cc cached_images_list_test.cc
                       cache_invariants_test.cc main.cc)
find_package(GTest REQUIRED)
find_package(Qt5 COMPONENTS Widgets Concurrent REQUIRED)
target_link_libraries(testing GTest::GTest Qt5::Widgets Qt5::Concurrent lib)
