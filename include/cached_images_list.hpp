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
#include <algorithm>

#include "abstract_image_cache.hpp"
#include "abstract_image_location.hpp"

using update_image_t = std::function<void(QPixmap const&)>;
using screen_width_t = std::function<int()>;

class CachedImagesList : public Abstract::ImageCache<QString, QPixmap> {
 public:
  CachedImagesList(std::size_t, info_t const&, update_image_t, screen_width_t);
  // Toggles between native (1:1 source) and the fit-to-width * zoom view.
  void scale();
  // The zoom factor is shared across all cached images (a property of the
  // comparison session, not of one image): the displayed width is
  // zoom_ * screen_width. Flipping between images therefore keeps the same
  // on-screen scale, so a given content region lines up across the switch.
  void zoomBy(double factor);
  void resetZoom();
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
  // On-screen width of the scaled view: zoom_ * screen_width, clamped so a huge
  // zoom can't allocate an unbounded pixmap (this view re-rasterizes the whole
  // image rather than transforming a view).
  int TargetWidth() const;
  QPixmap ScaleImageToWidth(int index);
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
  static constexpr double kMinZoom = 0.1;
  static constexpr double kMaxZoom = 8.0;
  static constexpr int kMaxScaledWidth = 8192;

  std::function<int()> ScreenWidth;
  bool is_scaled_;
  double zoom_;
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
      is_scaled_(false),
      zoom_(1.0) {}

inline void CachedImagesList::Clear() {
  source_.clear();
  scaled_.clear();
  // Outstanding decodes are simply dropped; their results are pulled by slot
  // index, so a discarded future can never land in a stale slot.
  pending_.clear();
}

inline void CachedImagesList::scale() { is_scaled_ = !is_scaled_; }

inline void CachedImagesList::zoomBy(double factor) {
  zoom_ = std::clamp(zoom_ * factor, kMinZoom, kMaxZoom);
  is_scaled_ = true;  // zooming implies the scaled view
}

inline void CachedImagesList::resetZoom() {
  zoom_ = 1.0;  // back to plain fit-to-width
  is_scaled_ = true;
}

inline int CachedImagesList::TargetWidth() const {
  const int width = static_cast<int>(zoom_ * ScreenWidth());
  return std::clamp(width, 1, kMaxScaledWidth);
}

inline void CachedImagesList::UpdateCurrentScaledImage() {
  scaled_[index()] = ScaleImageToWidth(index());
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
  // Recompute when the cached scaled pixmap no longer matches the current
  // target width (screen change or a zoom step invalidated it).
  if (scaled_.at(index).size().width() != TargetWidth()) {
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

inline QPixmap CachedImagesList::ScaleImageToWidth(int index) {
  return ResolvedSource(index).scaledToWidth(TargetWidth(),
                                             Qt::SmoothTransformation);
}