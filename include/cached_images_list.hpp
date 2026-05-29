#pragma once

#include <QCoreApplication>
#include <QDebug>
#include <QFuture>
#include <QImage>
#include <QLabel>
#include <QObject>
#include <QPainter>
#include <QScreen>
#include <QtConcurrent>

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
    pending_.pop_front();
  }
  void PopBack() override {
    source_.pop_back();
    scaled_.pop_back();
    pending_.pop_back();
  }
  std::size_t Size() const override { return source_.size(); }
  // Push only schedules the decode on the global thread pool and stores an
  // empty placeholder. The expensive QImage decode runs off the GUI thread;
  // the QPixmap is materialized lazily on first access (see ResolvedSource).
  void Push(pointer_t op, info_t value) override {
    constexpr pointer_t push_front_op = &QList<QPixmap>::push_front;
    const QString path = *value;
    qDebug() << "-- Caching (async)" << path;
    QFuture<QImage> future = QtConcurrent::run([path] {
      QImage image(path);
      if (image.isNull()) qWarning() << "Failed to load image:" << path;
      return image;
    });
    (source_.*op)(QPixmap{});
    (scaled_.*op)(QPixmap{});
    op == push_front_op ? pending_.push_front(future)
                        : pending_.push_back(future);
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
  // Materializes the QPixmap for a slot the first time it is needed. Blocks on
  // the decode future only if that particular image is not ready yet.
  const QPixmap& ResolvedSource(int index);
  // Visible stand-in shown instead of a blank screen when an image cannot be
  // decoded (missing/corrupt file). Must be built on the GUI thread.
  static QPixmap ErrorPlaceholder();
  void Clear() override;

  std::function<void(QPixmap const&)> UpdateImage;
  std::function<void()> save_scroll_position_;
  std::function<void(QSize)> restore_scroll_position_;
  std::function<int()> ScreenWidth;
  bool is_scaled_;
  QList<QPixmap> scaled_;
  QList<QPixmap> source_;
  QList<QFuture<QImage>> pending_;
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
  // Outstanding decodes are simply dropped; their results are pulled by slot
  // index, so a discarded future can never land in a stale slot.
  pending_.clear();
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
  // Held by value (QPixmap is copy-on-write): processEvents() below may run
  // navigation that mutates the cache, which would dangle a reference here.
  QPixmap const image = Image(index());
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

inline const QPixmap& CachedImagesList::ResolvedSource(int index) {
  if (source_.at(index).isNull()) {
    QImage image = pending_.at(index).result();
    source_[index] =
        image.isNull() ? ErrorPlaceholder() : QPixmap::fromImage(image);
  }
  return source_.at(index);
}

inline QPixmap CachedImagesList::ErrorPlaceholder() {
  QPixmap placeholder(640, 360);
  placeholder.fill(QColor(32, 32, 32));
  QPainter painter(&placeholder);
  painter.setPen(QColor(200, 200, 200));
  painter.drawText(placeholder.rect(), Qt::AlignCenter,
                   QStringLiteral("⚠  Failed to load image"));
  return placeholder;
}

inline const QPixmap& CachedImagesList::Image(int index) {
  return is_scaled_ ? ScaledImage(index) : ResolvedSource(index);
}

inline QPixmap CachedImagesList::ScaleImageToScreenWidth(int index) {
  return ResolvedSource(index).scaledToWidth(ScreenWidth(),
                                             Qt::SmoothTransformation);
}