set(HEADER_LIST "${photo_viewer_SOURCE_DIR}/include/main_window.hpp"
                "${photo_viewer_SOURCE_DIR}/include/lists.hpp"
                "${photo_viewer_SOURCE_DIR}/include/images_navigator.hpp"
                "${photo_viewer_SOURCE_DIR}/include/messagebox.hpp"
                "${photo_viewer_SOURCE_DIR}/include/images_selector_dialog.hpp"
                "${photo_viewer_SOURCE_DIR}/include/arrow_keys_scroller.hpp"
                "${photo_viewer_SOURCE_DIR}/include/global_path.hpp")

set(SOURCES_LIST "${photo_viewer_SOURCE_DIR}/src/main_window.cc"
                 "${photo_viewer_SOURCE_DIR}/src/arrow_keys_scroller.cc"
                 "${photo_viewer_SOURCE_DIR}/src/images_selector_dialog.cpp")

find_package(Qt5 COMPONENTS Widgets Network)
add_library(lib OBJECT ${SOURCES_LIST} ${HEADER_LIST})
target_link_libraries(lib PUBLIC Qt5::Widgets PRIVATE Qt5::Network)
target_include_directories(lib PUBLIC ../include/)
target_compile_features(lib PUBLIC cxx_std_20)
