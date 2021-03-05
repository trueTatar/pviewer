set(HEADER_LIST "${photo_viewer_SOURCE_DIR}/include/DownloadingDialog.hpp"
                "${photo_viewer_SOURCE_DIR}/include/PhotoScroller.hpp"
                "${photo_viewer_SOURCE_DIR}/include/PhotoViewer.hpp"
                "${photo_viewer_SOURCE_DIR}/include/PictureDownloader.hpp"
                "${photo_viewer_SOURCE_DIR}/include/SelectingDialog.hpp"
                "${photo_viewer_SOURCE_DIR}/include/global_path.hpp")

find_package(Qt5 COMPONENTS Widgets Network)
find_package(OpenSSL)
add_library( lib DownloadingDialog.cpp PhotoScroller.cpp PhotoViewer.cpp PictureDownloader.cpp SelectingDialog.cpp CachedImageList.cpp ${HEADER_LIST})
target_link_libraries(lib PRIVATE Qt5::Widgets PRIVATE Qt5::Network PRIVATE OpenSSL::SSL)
target_include_directories(lib PUBLIC ../include/)
target_compile_features(lib PUBLIC cxx_std_20)