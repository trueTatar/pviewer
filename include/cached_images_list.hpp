#pragma once

#include <QCoreApplication>
#include <QDebug>
#include <QLabel>
#include <QObject>
#include <QScreen>
#include <future>

#include "abstract_image_cache.hpp"
#include "abstract_image_location.hpp"

using update_image_t = std::function<void(QPixmap const&)>;
using screen_width_t = std::function<int()>;

class CachedImagesList : public Abstract::ImageCache<QString, QPixmap> {
 public:
  CachedImagesList(std::size_t, info_t const&, update_image_t, screen_width_t);
  void scale();
  bool isEmpty() const override { return source_.isEmpty(); }

  void DisplayImage() override;
  void HideImage() override;
  void UpdateCurrentScaledImage() override;

  void PopFront() override {
    source_.pop_front();
    scaled_.pop_front();
  }
  void PopBack() override {
    source_.pop_back();
    scaled_.pop_back();
  }
  std::size_t Size() const override { return source_.size(); }
  void Push(pointer_t op, info_t value) override {
    qDebug() << "-- Caching";
    qDebug() << "Image:" << *value;
    qDebug() << "Cache size before operation:" << source_.size();
    (source_.*op)(*value);
    (scaled_.*op)(QPixmap{});
    qDebug() << "Cache size after operation:" << source_.size();
  }
  // TODO give some normal, descriptive name
  void SetImageSizePasser(std::function<void()> save,
                          std::function<void(QSize)> restore) {
    save_scroll_position_ = save;
    restore_scroll_position_ = restore;
  }

 private:
  QPixmap ScaleImageToScreenWidth(int index);
  const QPixmap& Image(int index);
  const QPixmap& ScaledImage(int index);
  void Clear() override;

  std::function<void(QPixmap const&)> UpdateImage;
  std::function<void()> save_scroll_position_;
  std::function<void(QSize)> restore_scroll_position_;
  std::function<int()> ScreenWidth;
  bool is_scaled_;
  QList<QPixmap> scaled_;
  QList<QPixmap> source_;
};

inline CachedImagesList::CachedImagesList(std::size_t capacity,
                                          info_t const& image,
                                          update_image_t update_image,
                                          screen_width_t screen_width)
    : ImageCache(image, capacity),
      UpdateImage(update_image),
      ScreenWidth(screen_width),
      is_scaled_(false) {}

inline void CachedImagesList::Clear() {
  source_.clear();
  scaled_.clear();
}

inline void CachedImagesList::scale() { is_scaled_ = !is_scaled_; }

inline void CachedImagesList::UpdateCurrentScaledImage() {
  scaled_[index()] = ScaleImageToScreenWidth(index());
}

inline void CachedImagesList::DisplayImage() {
  qDebug() << "-- Displaying" << (is_scaled_ ? "[scaled]" : "[source]");
  qDebug() << "Image:" << CurrentImageLocation();
  qDebug() << "Image index in cache:" << index();
  qDebug() << "Size of cache:" << source_.size();
  QPixmap const& image = Image(index());
  save_scroll_position_();
  UpdateImage(image);
  // TODO replace with QEventLoop ???
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  if (!image.isNull()) restore_scroll_position_(image.size());
}

inline void CachedImagesList::HideImage() {
  save_scroll_position_();
  UpdateImage(QPixmap{});
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
  return source_.at(index).scaledToWidth(ScreenWidth(),
                                         Qt::SmoothTransformation);
}