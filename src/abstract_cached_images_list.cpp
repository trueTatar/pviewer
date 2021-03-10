#include "abstract_cached_images_list.hpp"

/*
**  AbstractCachedImagesList
 */

int AbstractCachedImagesList::OutdatedImagePos() const {
  return cacheSize() - (pos_ + 1);
}

bool AbstractCachedImagesList::isFilled() const {
  return size() == cacheSize();
}

void AbstractCachedImagesList::Reset() {
  Clear();
  pos_ = 0;
}

int AbstractCachedImagesList::index() const {
  return pos_;
}

void AbstractCachedImagesList::setIndex(int value) {
  pos_ = value;
}

void AbstractCachedImagesList::MoveIndex(int step) {
  bool increment = pos_ < cacheSize() - 1;
  bool decrement = pos_ > 0;
  if ((step == 1 && increment) || (step == -1 && decrement))
    pos_ = pos_ + step;
}

/*
**  CachedImagesListImpl
 */

