#pragma once

#include <QList>
#include <QVector>

namespace Abstract {

template<typename In, typename Im>
class TaskQueue {
protected:
  using info_t = typename QVector<In>::iterator;
  using pointer_t = void(QList<Im>::*)(const Im&);
public:
  TaskQueue(const int initial_task_size): initial_task_size_(initial_task_size) { }
  template<pointer_t potter>
  void Push(info_t iter) {
    PushBack(iter);
    operation_pointer_.push_back(potter);
  }
  int InitialTaskSize() {
    return initial_task_size_;
  }
  std::pair<info_t, pointer_t> Next() {
    pointer_t p = operation_pointer_.front();
    operation_pointer_.pop_front();
    return { PopFront(), p };
  }
  virtual ~TaskQueue() = default;
  virtual void PushBack(info_t) = 0;
  virtual info_t PopFront() = 0;
  virtual bool Empty() const = 0;
private:
  const int initial_task_size_;
  QList<pointer_t> operation_pointer_;
};

}