#pragma once

#include <memory>

#include <QObject>

#include "order.hpp"
#include "cached_images_list.hpp"

namespace Abstract {

/*
 *  The Abstract::ImageLocation class is an abstract class, that provides a base implementation
 *  for classes that hold and modify information about the location of images on the hard drive.
 */
  template<typename In, typename Im>
  class ImageLocation: public QObject {

    struct First { };
    struct Second { };

    template<typename T> void MoveIndex(int step);
    template<typename Order> bool Increment();
    template<typename Order> bool Decrement();
  protected:
    template<typename... Ts>
    using info_storage_t = QVector<Ts...>;
    template<typename... Ts>
    using image_storage_t = QList<Ts...>;

    using task_queue_t = TaskQueue<In, Im>;
    using info_t = typename info_storage_t<In>::iterator;
    using const_info_t = typename info_storage_t<In>::const_iterator;
    using pointer_t = void(image_storage_t<Im>::*)(const Im&);
    static constexpr pointer_t Back = &image_storage_t<Im>::push_back;
    static constexpr pointer_t Front = &image_storage_t<Im>::push_front;

  public:
    ImageLocation(info_t value, std::unique_ptr<task_queue_t> = std::make_unique<::TaskQueue>());
    template<typename Order> bool pastTheLastIsPointed();
    template<typename Order> bool initialState();
    template<typename Order> bool lastIsPointed();
    template<typename Object, typename... Ts>
    auto CreateCacheObject(std::size_t capacity, Ts&&... ts) {
      return std::make_shared<Object>(capacity, index_, std::forward<Ts>(ts)...);
    }
    task_queue_t& TaskQueue() {
      return *task_queue_;
    }
    void MoveIndex(int step);
    void MoveIndex(int step, bool is_image_null);
    int Pos() const {
      return std::distance(CBegin(), const_info_t(index_));
    }
    info_t Index() {
      return index_;
    }
    const_info_t Index() const {
      return index_;
    }
    void setIndex(int value) {
      index_ = std::next(Begin(), value);
    }
    static void InitialImageTask(ImageLocation& file_info, task_queue_t& task_queue) {
      if (file_info.isEmpty()) return;
      task_queue.template Push<nullptr>(file_info.Index());
      auto left_edge = std::prev(file_info.Index());
      auto right_edge = std::next(file_info.Index());
      auto BeyondBorders = [ & ] () mutable {
        return left_edge == std::prev(file_info.Begin()) && right_edge == file_info.End();
      };
      int queue_initial_size = 5;
      for (int i = queue_initial_size - 1; i > 0 && !BeyondBorders();) {
        if (right_edge != file_info.End()) {
          --i;
          task_queue.template Push<Back>(right_edge);
          std::advance(right_edge, 1);
        }
        if (left_edge != std::prev(file_info.Begin())) {
          --i;
          task_queue.template Push<Front>(left_edge);
          std::advance(left_edge, -1);
        }
      }
    }

    virtual bool isEmpty() const = 0;
    virtual int size() const = 0;
    virtual info_t Begin() = 0;
    virtual info_t End() = 0;
  protected:
    void setNewList(info_storage_t<QString>& list) {
      index_ = list.begin();
      InitialImageTask(*this, *task_queue_);
    }
  private:
    virtual const_info_t CBegin() const = 0;
    std::unique_ptr<task_queue_t> task_queue_;
    info_t index_;
  };


  template<typename In, typename Im>
  template<typename T>
  void ImageLocation<In, Im>::MoveIndex(int step) {
    if ((step == 1 && Increment<T>()) || (step == -1 && Decrement<T>()))
      std::advance(index_, step);
  }

  template<typename In, typename Im>
  template<typename Order>
  bool ImageLocation<In, Im>::Increment() {
    if constexpr (std::is_same_v<First, Order>)
      return index_ < End();
    else if constexpr (std::is_same_v<Second, Order>)
      return index_ == std::prev(Begin());
  }

  template<typename In, typename Im>
  template<typename Order>
  bool ImageLocation<In, Im>::Decrement() {
    if constexpr (std::is_same_v<First, Order>)
      return index_ > std::prev(Begin());
    else if constexpr (std::is_same_v<Second, Order>)
      return index_ == End();
  }

  template<typename In, typename Im>
  inline ImageLocation<In, Im>::ImageLocation(info_t value, std::unique_ptr<task_queue_t> task_queue)
    : task_queue_(std::move(task_queue))
    , index_(value) { }

  template<typename In, typename Im>
  template<typename Order>
  bool ImageLocation<In, Im>::pastTheLastIsPointed() {
    if constexpr (std::is_same_v<NumericalOrder, Order>)
      return index_ == End();
    else if constexpr (std::is_same_v<ReverseOrder, Order>)
      return index_ == std::prev(Begin());
  }

  template<typename In, typename Im>
  template<typename Order>
  bool ImageLocation<In, Im>::initialState() {
    if constexpr (std::is_same_v<NumericalOrder, Order>)
      return index_ == std::prev(Begin());
    else if constexpr (std::is_same_v<ReverseOrder, Order>)
      return index_ == End();
  }

  template<typename In, typename Im>
  template<typename Order>
  bool ImageLocation<In, Im>::lastIsPointed() {
    if constexpr (std::is_same_v<NumericalOrder, Order>)
      return index_ == std::prev(End());
    else if constexpr (std::is_same_v<ReverseOrder, Order>)
      return index_ == Begin();
  }

  template<typename In, typename Im>
  inline void ImageLocation<In, Im>::MoveIndex(int step) {
    MoveIndex<First>(step);
  }

  template<typename In, typename Im>
  inline void ImageLocation<In, Im>::MoveIndex(int step, bool is_image_null) {
    is_image_null ? MoveIndex<Second>(step) : MoveIndex<First>(step);
  }

}

class CPathBase: public Abstract::ImageLocation<QString, QPixmap> {
public:
  CPathBase(int value, info_storage_t<QString> container = {})
    : ImageLocation(std::next(container.begin(), value))
    , container_(std::move(container)) { }
  QString const& pathByIndex() const;
  void setNewList(info_storage_t<QString>);
  void appendItem(QString path);
  void clear() {
    container_.clear();
  }
  bool isEmpty() const final {
    return container_.isEmpty();
  }
  int size() const override {
    return container_.size();
  }

  info_t Begin() override {
    return container_.begin();
  }
  info_t End() override {
    return container_.end();
  }
  const_info_t CBegin() const override {
    return container_.cbegin();
  }

private:
  info_storage_t<QString> container_;
};

inline QString const& CPathBase::pathByIndex() const {
  return *Index();
}

inline void CPathBase::setNewList(info_storage_t<QString> list) {
  container_ = std::move(list);
  ImageLocation::setNewList(container_);
}

inline void CPathBase::appendItem(QString path) {
  container_.append(path);
}
