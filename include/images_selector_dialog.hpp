#pragma once

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QFileDialog>

#include "global_path.hpp"
#include "images_navigator.hpp"
#include "lists.hpp"

class ImagesSelectorDialog : public QDialog {
  Q_OBJECT
  enum class GetDirectoryVia { k_Supplied, k_DialogWindow };

 public:
  ImagesSelectorDialog(QWidget* = nullptr);
  void AssociateWith(std::shared_ptr<FolderPath> p) {
    connect(p.get(), &FolderPath::folderIsChanged, [this](QDir dir) {
      selectAllImagesInDirectory(GetDirectoryVia::k_Supplied, dir);
    });
  }
  void setDirectory(QString, int);
  void setImages(QList<QString>);

 protected:
  void keyPressEvent(QKeyEvent*) override;

 private:
  void convertPathsToAbsolute(QDir, QList<QString>&);
  void sortItems(QList<QString>&);
  QPushButton* createButton(QString);
  QPushButton* createButton(QString, void (ImagesSelectorDialog::*)());
  void selectImages();
  void selectAllImagesInSubdirectories();
  void selectAllImagesInDirectory(GetDirectoryVia, QDir, int = 1);

  QCheckBox* clipboard_checkbox_;

 signals:
  void updateFolderList(QStringList);
  void stringListPrepared(QList<QString>, int);
};