#pragma once

#include <QScreen>
#include <QLabel>
#include <QObject>

#include <QDebug>

#include <future>

#include "abstract_cached_images_list.hpp"
#include "abstract_paths.hpp"

using update_image_t = std::function<void(QPixmap const&)>;
using screen_width_t = std::function<int()>;

class CachedImagesList: public Abstract::ImageCache<QString, QPixmap> {
public:
  CachedImagesList(std::size_t, info_t const&, update_image_t, screen_width_t);
  void scale();
  bool isEmpty() const override {
    return source_.isEmpty();
  }
  void Clear() override;

  void CacheImage() override;
  void DisplayImage() override;
  void HideImage() override;
  void UpdateCurrentScaledImage() override;
  int RemoveOutdatedImage() override;

  void PopFront() override {
    source_.pop_front();
  }
  void PopBack() override {
    source_.pop_back();
  }
  std::size_t Size() const override {
    return source_.size();
  }
  void Push(pointer_t op, info_t value) override {
    (source_.*op)(*value);
  }
private:
  QPixmap ScaleImageToScreenWidth(int index);
  const QPixmap& Image(int index);
  const QPixmap& ScaledImage(int index);

  std::function<void(QPixmap const&)> UpdateImage;
  std::function<int()> ScreenWidth;
  bool is_scaled_;
  QList<QPixmap> scaled_;
  QList<QPixmap> source_;
};

inline CachedImagesList::CachedImagesList(std::size_t capacity, info_t const& image, update_image_t update_image, screen_width_t screen_width)
  : ImageCache(image, capacity)
  , UpdateImage(update_image)
  , ScreenWidth(screen_width)
  , is_scaled_(false) { }

inline void CachedImagesList::Clear() {
  source_.clear();
  scaled_.clear();
}

inline void CachedImagesList::scale() {
  is_scaled_ = !is_scaled_;
}

inline void CachedImagesList::UpdateCurrentScaledImage() {
  scaled_[index()] = ScaleImageToScreenWidth(index());
}

inline void CachedImagesList::DisplayImage() {
  QPixmap const& image = Image(index());
  UpdateImage(image);
}

inline void CachedImagesList::HideImage() {
  UpdateImage(QPixmap {});
}

inline void CachedImagesList::CacheImage() {
  source_.insert(index(), ImagePath());
  auto b = std::async([ this ] () { scaled_.insert(index(), ScaleImageToScreenWidth(index())); });
}

inline QPixmap const& CachedImagesList::ScaledImage(int index) {
  int image_width = scaled_.at(index).size().width();
  if (image_width != ScreenWidth()) {
    UpdateCurrentScaledImage();
  }
  return scaled_.at(index);
}

inline const QPixmap& CachedImagesList::Image(int index) {
  return is_scaled_ ? ScaledImage(index) : source_.at(index);
}

inline QPixmap CachedImagesList::ScaleImageToScreenWidth(int index) {
  return source_.at(index).scaledToWidth(ScreenWidth(), Qt::SmoothTransformation);
}

inline int CachedImagesList::RemoveOutdatedImage() {
  int position = OutdatedImagePos();
  source_.removeAt(position);
  scaled_.removeAt(position);
  return position;
}
