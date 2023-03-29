#pragma once

#include <QDir>
#include <QLabel>
#include <QScreen>
#include <QString>

#include "abstract_image_location.hpp"

class ImagePath : public CPathBase {
 public:
  ImagePath() : CPathBase(-1) {}
  QString imageNumber() {
    QString info;
    if (initialState<NumericalOrder>()) {
      info = "beginning of list";
    } else if (initialState<ReverseOrder>()) {
      info = "end of list";
    } else {
      QString current = QString::number(Pos() + 1);
      QString in_total = QString::number(size());
      info = current + " / " + in_total;
    }
    return info;
  }
};

class FolderPath : public CPathBase {
  Q_OBJECT
 public:
  FolderPath() : CPathBase(0) {}
  void sendNewFolder() {
    QDir dir = pathByIndex();
    emit folderIsChanged(dir);
  }
 signals:
  void folderIsChanged(QDir);
};
