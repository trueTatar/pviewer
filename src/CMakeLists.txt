set(HEADER_LIST "${photo_viewer_SOURCE_DIR}/include/DownloadingDialog.hpp"
                "${photo_viewer_SOURCE_DIR}/include/scroller.h"
                "${photo_viewer_SOURCE_DIR}/include/lists.h"
                "${photo_viewer_SOURCE_DIR}/include/move.h"
                "${photo_viewer_SOURCE_DIR}/include/messagebox.h"
                "${photo_viewer_SOURCE_DIR}/include/PictureDownloader.hpp"
                "${photo_viewer_SOURCE_DIR}/include/SelectingDialog.hpp"
                "${photo_viewer_SOURCE_DIR}/include/global_path.hpp")

set(SOURCES_LIST "${photo_viewer_SOURCE_DIR}/src/DownloadingDialog.cpp"
                 "${photo_viewer_SOURCE_DIR}/src/scroller.cc"
                 "${photo_viewer_SOURCE_DIR}/src/PictureDownloader.cpp"
                 "${photo_viewer_SOURCE_DIR}/src/SelectingDialog.cpp")

find_package(Qt5 COMPONENTS Widgets Network)
find_package(OpenSSL)
add_library(lib OBJECT ${SOURCES_LIST} ${HEADER_LIST})
target_link_libraries(lib PUBLIC Qt5::Widgets PRIVATE Qt5::Network OpenSSL::SSL)
target_include_directories(lib PUBLIC ../include/)
target_compile_features(lib PUBLIC cxx_std_20)
