#pragma once

#include <QObject>
#include <memory>

#include "cached_images_list.hpp"

namespace Abstract {

/*
 *  The Abstract::ImageLocation class is an abstract class, that provides a base
 * implementation for classes that hold and modify information about the
 * location of images on the hard drive.
 */
template <typename In, typename Im>
class ImageLocation : public QObject {
 protected:
  template <typename... Ts>
  using info_storage_t = QVector<Ts...>;
  template <typename... Ts>
  using image_storage_t = QList<Ts...>;

  using task_queue_t = TaskQueue<In, Im>;
  using info_t = typename info_storage_t<In>::iterator;
  using const_info_t = typename info_storage_t<In>::const_iterator;
  using pointer_t = void (image_storage_t<Im>::*)(const Im &);
  static constexpr pointer_t Back = &image_storage_t<Im>::push_back;
  static constexpr pointer_t Front = &image_storage_t<Im>::push_front;

 public:
  ImageLocation(info_t value);
  void MoveIndex(int step);
  template <typename Order>
  bool initialState();
  template <typename Order>
  bool lastIsPointed();
  template <typename Object, typename... Ts>
  auto CreateCacheObject(std::size_t capacity, Ts &&...ts) {
    auto image_cache_ptr =
        std::make_shared<Object>(capacity, index_, std::forward<Ts>(ts)...);
    Object &image_cache = *image_cache_ptr.get();
    CacheImage = [this, &image_cache] {
      image_cache.ProcessTaskQueue(*task_queue_.get());
    };
    return image_cache_ptr;
  }
  task_queue_t &TaskQueueObject() { return *task_queue_; }
  int Pos() const { return std::distance(CBegin(), const_info_t(index_)); }
  info_t Index() { return index_; }
  const_info_t Index() const { return index_; }
  void setIndex(int value) { index_ = std::next(Begin(), value); }
  bool ImageHideNeeded(int direction) {
    bool from_begin = index_ == Begin() && direction < 0;
    bool from_end = index_ == std::prev(End()) && direction > 0;
    return from_begin || from_end;
  }
  bool DisplayCachedImageNeeded(int direction) {
    bool from_begin = index_ == Begin() && direction > 0;
    bool from_end = index_ == std::prev(End()) && direction < 0;
    bool from_middle = Begin() < index_ && index_ < End();
    return from_begin || from_end || from_middle;
  }
  static void InitialImageTask(ImageLocation &file_info,
                               task_queue_t &task_queue) {
    if (file_info.isEmpty()) return;
    task_queue.template Push<nullptr>(file_info.Index());
    auto left_edge = std::prev(file_info.Index());
    auto right_edge = std::next(file_info.Index());
    auto BeyondBorders = [&]() mutable {
      return left_edge == std::prev(file_info.Begin()) &&
             right_edge == file_info.End();
    };
    for (int i = task_queue.InitialTaskSize() - 1; i > 0 && !BeyondBorders();) {
      if (i > 0 && right_edge != file_info.End()) {
        --i;
        task_queue.template Push<Back>(right_edge);
        std::advance(right_edge, 1);
      }
      if (i > 0 && left_edge != std::prev(file_info.Begin())) {
        --i;
        task_queue.template Push<Front>(left_edge);
        std::advance(left_edge, -1);
      }
    }
  }

  template <typename TaskQueueObject>
  void CreateTaskQueue(const int initial_task_size) {
    task_queue_ = std::make_unique<TaskQueueObject>(initial_task_size);
  }

  virtual bool isEmpty() const = 0;
  virtual int size() const = 0;
  virtual info_t Begin() = 0;
  virtual info_t End() = 0;
  std::function<void()> CacheImage;

 private:
  virtual const_info_t CBegin() const = 0;
  std::unique_ptr<task_queue_t> task_queue_;
  info_t index_;
};

template <typename In, typename Im>
void ImageLocation<In, Im>::MoveIndex(int direction) {
  bool increment_needed = index_ < std::prev(End()) && direction == 1;
  bool decrement_needed = std::prev(Begin()) < index_ && direction == -1;
  if (increment_needed || decrement_needed) std::advance(index_, direction);
}

template <typename In, typename Im>
inline ImageLocation<In, Im>::ImageLocation(info_t value) : index_(value) {}

template <typename In, typename Im>
template <typename Order>
bool ImageLocation<In, Im>::initialState() {
  if constexpr (std::is_same_v<NumericalOrder, Order>)
    return index_ == std::prev(Begin());
  else if constexpr (std::is_same_v<ReverseOrder, Order>)
    return index_ == End();
}

template <typename In, typename Im>
template <typename Order>
bool ImageLocation<In, Im>::lastIsPointed() {
  if constexpr (std::is_same_v<NumericalOrder, Order>)
    return index_ == std::prev(End());
  else if constexpr (std::is_same_v<ReverseOrder, Order>)
    return index_ == Begin();
}

}  // namespace Abstract

class CPathBase : public Abstract::ImageLocation<QString, QPixmap> {
 public:
  CPathBase(int value, info_storage_t<QString> container = {})
      : ImageLocation(std::next(container.begin(), value)),
        container_(std::move(container)) {}
  QString const &pathByIndex() const;
  void setNewList(info_storage_t<QString>);
  void appendItem(QString path);
  void clear() { container_.clear(); }
  bool isEmpty() const final { return container_.isEmpty(); }
  int size() const override { return container_.size(); }

  info_t Begin() override { return container_.begin(); }
  info_t End() override { return container_.end(); }
  const_info_t CBegin() const override { return container_.cbegin(); }

 private:
  info_storage_t<QString> container_;
};

inline QString const &CPathBase::pathByIndex() const { return *Index(); }

inline void CPathBase::setNewList(info_storage_t<QString> list) {
  container_ = std::move(list);
  ImageLocation::setIndex(0);
}

inline void CPathBase::appendItem(QString path) { container_.append(path); }