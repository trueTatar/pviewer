#include "PhotoViewer.hpp"
#include <QDebug>

QPixmap CachedImageList::scaleImageToScreenWidth() {
  auto screenWidth = widget_.screen()->size().width();
  return container_.at(index_).scaledToWidth(screenWidth, Qt::SmoothTransformation);
}

void CachedImageList::cacheCurrentImage() {
  container_.insert(index_, std::move(QPixmap(paths_->pathByIndex())) );
  auto screenWidth = widget_.screen()->size().width();
  scaled_.insert(index_, std::move(scaleImageToScreenWidth()));
}