#pragma once

#include <QCoreApplication>
#include <QDebug>
#include <QFuture>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QtConcurrent>

#include "abstract_image_cache.hpp"
#include "abstract_image_location.hpp"

using update_image_t = std::function<void(QPixmap const&)>;

// Zoom and scaling now live entirely in the view layer (MainWindow / QGraphicsView).
// CachedImagesList is responsible only for: async decode, cache eviction, and
// handing source pixmaps to the view via the update_image callback.
class CachedImagesList : public Abstract::ImageCache<QString, QPixmap> {
 public:
  CachedImagesList(std::size_t capacity, info_t const&, update_image_t);
  bool isEmpty() const override { return source_.isEmpty(); }

  void DisplayImage() override;
  void HideImage() override;

  void PopFront() override {
    source_.pop_front();
    pending_.pop_front();
  }
  void PopBack() override {
    source_.pop_back();
    pending_.pop_back();
  }
  std::size_t Size() const override { return source_.size(); }
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
    op == push_front_op ? pending_.push_front(future)
                        : pending_.push_back(future);
  }
  void SetScrollCallbacks(std::function<void()> save,
                          std::function<void()> restore) {
    save_scroll_position_ = save;
    restore_scroll_position_ = restore;
  }

 private:
  // Materializes the QPixmap for a slot the first time it is needed. Blocks on
  // the decode future only if that particular image is not ready yet.
  const QPixmap& ResolvedSource(int index);
  // Visible stand-in shown instead of a blank screen when an image cannot be
  // decoded (missing/corrupt file). Must be built on the GUI thread.
  static QPixmap ErrorPlaceholder();
  void Clear() override;

  std::function<void(QPixmap const&)> UpdateImage;
  std::function<void()> save_scroll_position_;
  std::function<void()> restore_scroll_position_;
  QList<QPixmap> source_;
  QList<QFuture<QImage>> pending_;
};

inline CachedImagesList::CachedImagesList(std::size_t capacity,
                                          info_t const& image,
                                          update_image_t update_image)
    : ImageCache(image, capacity), UpdateImage(update_image) {}

inline void CachedImagesList::Clear() {
  source_.clear();
  // Outstanding decodes are simply dropped; their results are pulled by slot
  // index, so a discarded future can never land in a stale slot.
  pending_.clear();
}

inline void CachedImagesList::DisplayImage() {
  qDebug() << "-- Displaying";
  qDebug() << "Image:" << CurrentImageLocation();
  qDebug() << "Image index in cache:" << index();
  qDebug() << "Size of cache:" << source_.size();
  // Held by value (QPixmap is copy-on-write): processEvents() below may run
  // navigation that mutates the cache, which would dangle a reference here.
  QPixmap const image = ResolvedSource(index());
  save_scroll_position_();
  UpdateImage(image);
  restore_scroll_position_();
  QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

inline void CachedImagesList::HideImage() {
  save_scroll_position_();
  UpdateImage(QPixmap{});
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
