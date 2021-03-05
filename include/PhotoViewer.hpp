#pragma once

#include <QLabel>
#include <QString>
#include <QScreen>

enum Pixmap {
  NextImage = 0, PreviousImage
};
template<typename T>
class ListInterface {
protected:
  int index_;
  QList<T> container_;
public:
  int index() { return index_; }
  void setIndex(int value) { index_ = value; }
  bool isEmpty() const { return container_.empty(); }
  bool lastIsPointed(Pixmap cond) {
    bool res = false;
    if (cond == NextImage) {
      res = index() == size() - 1;
    }
    if (cond == PreviousImage) {
      res = index() == 0;
    }
    return res;
  }
  virtual ~ListInterface() = default;
  virtual int size() const { return container_.size(); }
  virtual void clear() { container_.clear(); }
  virtual void incrementIndex() = 0;
  virtual void decrementIndex() = 0;
};
class ImagePathList : public ListInterface<QString> {
public:
  ImagePathList() { setIndex(-1); }
  virtual const QString& pathByIndex() { return container_.at(index_); }
  void setNewList(const QList<QString>& list) { container_ = list; }
  bool pastTheLastIsPointed(Pixmap cond) {
    bool res = false;
    if (cond == NextImage) {
      res = index() == size();
    }
    if (cond == PreviousImage) {
      res = index() == -1;
    }
    return res;
  }
  bool initialState(Pixmap cond) {
    bool res = false;
    if (cond == NextImage) {
      res = index() == -1;
    }
    if (cond == PreviousImage) {
      res = index() == size();
    }
    return res;
  }
  void incrementIndex() override {
    if (index_ < container_.size())
      ++index_;
  }
  void decrementIndex() override {
    if (index_ >= 0)
      --index_;
  }
  void appendItem(QString path) {
    container_.append(path);
  }
};

class CachedImageList : public ListInterface<QPixmap> {
protected:
  QList<QPixmap> scaled_;
  bool imageIsScaled_;
  std::shared_ptr<ImagePathList> paths_;
  QWidget& widget_;
  QPixmap scaleImageToScreenWidth();
public:
  CachedImageList(QWidget& pv, std::shared_ptr<ImagePathList> p) : widget_(pv), paths_(p) {
    imageIsScaled_ = false;
    setIndex(-1);
  }
  const QPixmap& getImage() const { return imageIsScaled_ ? scaled_.at(index_) : container_.at(index_); }
  virtual int cacheSize() const { return 10; }
  void clear() override {
    container_.clear();
    scaled_.clear();
  }
  void scale() {
    imageIsScaled_ = !imageIsScaled_;
  }
  bool isImageScaled() const {
    return imageIsScaled_ ? true : false;
  }
  void updateCurrentScaledImage() {
    scaled_[index_] = scaleImageToScreenWidth();
  }
  virtual void cacheCurrentImage();
  virtual int removeOutdatedImage() {
    int position = cacheSize() - (index_ + 1);
    container_.removeAt(position);
    scaled_.removeAt(position);
    return position;
  }
  bool isFilled() const {
    return size() == cacheSize();
  }
  void incrementIndex() override {
    if (index_ < cacheSize() - 1)
      ++index_;
  }
  void decrementIndex() override {
    if (index_ > 0)
      --index_;
  }
};

class PhotoViewer : public QLabel {
  Q_OBJECT
private:
  virtual void formatWidget();
  virtual void createContainers();
  void setUpAttributes( Pixmap, int = 1 );

  int fillCache(Pixmap cond);
  int updateCache(Pixmap cond);
  int moveThroughCache(Pixmap cond);
  int setImageFromCache(Pixmap cond);
  int noAction(Pixmap cond);
  void moveFilePathsIndex(Pixmap cond);
protected:
  std::shared_ptr<ImagePathList> paths;
  std::shared_ptr<CachedImageList> cache;
public:
  PhotoViewer(QWidget* = nullptr);
  void prepareWidget(int = 1);
  void setImagePaths(QList<QString>);
  virtual bool pixmapIsNull() {
    return pixmap( Qt::ReturnByValue ).isNull();
  }
  void appendToImagePaths(QString);
  void repaintPixmap();
  int goTo(Pixmap);
  void scale();
  bool isScaled() { return cache->isImageScaled(); }
  void setFiles();
  void reset(Pixmap);
  QString imageNumber();

signals:
  void pixmapIsChanging();
};