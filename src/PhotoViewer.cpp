#include "PhotoViewer.hpp"

void PhotoViewer::createContainers()
{
  paths = std::make_shared<ImagePathList>();
  cache = std::make_shared<CachedImageList>(*this, paths);
}

void PhotoViewer::prepareWidget(int num)
{
  createContainers();
  setUpAttributes(NextImage, num);
  formatWidget();
}

PhotoViewer::PhotoViewer(QWidget* pwgt ) : QLabel( pwgt ) { }

void PhotoViewer::setImagePaths(QList<QString> path) {
  paths->setNewList(path);
}

void PhotoViewer::appendToImagePaths(QString path) {
  paths->appendItem(path);
}

int PhotoViewer::goTo(Pixmap cond)
{
  if (paths->isEmpty()) {
    return -1;
  }
  emit pixmapIsChanging();
  if (!cache->isEmpty() && paths->initialState(cond)) {
    return setImageFromCache(cond);
  }
  if (paths->lastIsPointed(cond) || paths->pastTheLastIsPointed(cond)) {
    return noAction(cond);
  }
  if (cache->isFilled() && cache->lastIsPointed(cond)) {
    return updateCache(cond);
  }
  if (!cache->isEmpty() && !cache->lastIsPointed(cond)) {
    return moveThroughCache(cond);
  }
  return fillCache(cond);

  resize( pixmap( Qt::ReturnByValue ).size() );
  return -1;
}

void PhotoViewer::moveFilePathsIndex(Pixmap cond) {
  if (cond == NextImage) {
    paths->incrementIndex();
  }
  if (cond == PreviousImage) {
    paths->decrementIndex();
  }
}

int PhotoViewer::fillCache(Pixmap cond) {
  if (cond == NextImage) {
    cache->incrementIndex();
  }
  moveFilePathsIndex(cond);
  cache->cacheCurrentImage();
  setPixmap(cache->getImage());
  return cond == NextImage ? 1 : 10;
}

int PhotoViewer::updateCache(Pixmap cond) {
  moveFilePathsIndex(cond);
  cache->removeOutdatedImage();
  cache->cacheCurrentImage();
  setPixmap(cache->getImage());
  return cond == NextImage ? 2 : 20;
}

int PhotoViewer::moveThroughCache(Pixmap cond) {
  if (cond == NextImage) {
    cache->incrementIndex();
  }
  if (cond == PreviousImage) {
    cache->decrementIndex();
  }
  moveFilePathsIndex(cond);
  setPixmap(cache->getImage());
  return cond == NextImage ? 3 : 30;
}

int PhotoViewer::setImageFromCache(Pixmap cond) {
  moveFilePathsIndex(cond);
  setPixmap(cache->getImage());
  return cond == NextImage ? 4 : 40;
}

int PhotoViewer::noAction(Pixmap cond) {
  moveFilePathsIndex(cond);
  if (!pixmapIsNull()) {
    setPixmap(QPixmap());
  }
  return cond == NextImage ? 5 : 50;
}

void PhotoViewer::scale() {
  if (!cache->isEmpty() && !pixmapIsNull()) {
    cache->scale();
    setPixmap(cache->getImage());
  }
}

void PhotoViewer::setFiles()
{
  if (!paths->isEmpty()) {
    reset(NextImage);
    paths->clear();
  }
}

void PhotoViewer::repaintPixmap()
{
  if (!pixmapIsNull()) {
    cache->updateCurrentScaledImage();
    setPixmap(cache->getImage());
  }
}

void PhotoViewer::setUpAttributes(Pixmap cond, int value)
{
  if (cond == NextImage) {
    value < 1 ? value = 1 : 42;
    paths->setIndex(value - 2);
    cache->setIndex(-1);
  }
  if (cond == PreviousImage) {
    paths->setIndex(paths->size());
    cache->setIndex(0);
  }
}

void PhotoViewer::reset(Pixmap cond)
{
  clear();
  cache->clear();
  setUpAttributes(cond);
}

QString PhotoViewer::imageNumber() {
  QString info;
  if (paths->initialState(NextImage)) {
    info = "beginning of list";
  } else {
    if (paths->initialState(PreviousImage)) {
      info = "end of list";
    } else {
      info = QString().number( paths->index() + 1 ) + " / " + QString().number( paths->size() );
    }
  }
  return info;
}

void PhotoViewer::formatWidget()
{
  setAlignment(Qt::AlignCenter);
  QPalette palette;
  int colorValue = 32;
  QColor darkGray( colorValue, colorValue, colorValue );
  palette.setColor( QPalette::Window, darkGray );
  setAutoFillBackground( true );
  setPalette( palette );
}