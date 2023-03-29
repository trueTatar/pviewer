#include "images_selector_dialog.hpp"

#include <QApplication>
#include <QCollator>
#include <QFileDialog>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

QPushButton* ImagesSelectorDialog::createButton(QString name,
                                           void (ImagesSelectorDialog::*pointer)()) {
  QPushButton* button = new QPushButton(name);
  connect(button, &QPushButton::clicked, this, pointer);
  return button;
}

QPushButton* ImagesSelectorDialog::createButton(QString name) {
  QPushButton* button = new QPushButton(name);
  if (name == "&Directory") {
    connect(button, &QPushButton::clicked, [&] {
      selectAllImagesInDirectory(GetDirectoryVia::k_DialogWindow, QDir());
    });
  } else if (name == "&Embedded") {
    connect(button, &QPushButton::clicked,
            [&] { selectAllImagesInSubdirectories(); });
  }
  return button;
}

ImagesSelectorDialog::ImagesSelectorDialog(QWidget* parent)
    : QDialog(parent, Qt::WindowCloseButtonHint),
      clipboard_checkbox_(new QCheckBox("&Clipboard", this)) {
  QGridLayout* buttonLayout = new QGridLayout(this);
  buttonLayout->addWidget(clipboard_checkbox_, 1, 0, 1, -1);
  buttonLayout->addWidget(
      createButton("&File(s)", &ImagesSelectorDialog::selectImages), 0, 0);
  buttonLayout->addWidget(createButton("&Directory"), 0, 1);
  buttonLayout->addWidget(createButton("&Embedded"), 0, 2);
}

void ImagesSelectorDialog::setDirectory(QString path, int pos) {
  if (!path.isEmpty()) {
    selectAllImagesInDirectory(GetDirectoryVia::k_Supplied, QDir(path), pos);
  }
}

void ImagesSelectorDialog::setImages(QList<QString> images) {
  emit stringListPrepared(images, 1);
}

void ImagesSelectorDialog::selectImages() {
  accept();
  QList<QString> pixmap_list = QFileDialog::getOpenFileNames(
      dynamic_cast<QWidget*>(parent()), "Select one or more files to open",
      g_basicPath, "Images (*.png *.jpg *.jpeg)");
  emit stringListPrepared(pixmap_list, 1);
}

void ImagesSelectorDialog::selectAllImagesInDirectory(GetDirectoryVia opt,
                                                 QDir directory, int pos) {
  accept();
  if (clipboard_checkbox_->isChecked()) {
    clipboard_checkbox_->setChecked(false);
    directory = QDir(QApplication::clipboard()->text());
  } else if (opt == GetDirectoryVia::k_DialogWindow) {
    QString s = "Select directory";
    directory = QDir(QFileDialog::getExistingDirectory(this, s, g_basicPath));
  }

  if (!directory.exists()) {
    qDebug() << "Error:" << directory.dirName() << "is not a valid directory.";
    return;
  }

  directory.setNameFilters(QStringList() << "*.jpg"
                                         << "*.jpeg"
                                         << "*.png");
  QList<QString> pixmap_list = directory.entryList(QDir::Files);
  sortItems(pixmap_list);
  convertPathsToAbsolute(directory, pixmap_list);
  emit stringListPrepared(pixmap_list, pos);
}

void ImagesSelectorDialog::selectAllImagesInSubdirectories() {
  QDir directory = QDir(
      QFileDialog::getExistingDirectory(this, "Select directory", g_basicPath));
  accept();

  if (!directory.exists()) {
    qDebug() << "Error:" << directory.dirName() << "is not a valid directory.";
    return;
  }

  QStringList dir_list = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  if (dir_list.empty()) {
    selectAllImagesInDirectory(GetDirectoryVia::k_Supplied, directory);
  } else {
    convertPathsToAbsolute(directory, dir_list);
    emit updateFolderList(dir_list);
    QString first_item = dir_list.at(0);
    selectAllImagesInDirectory(GetDirectoryVia::k_Supplied, QDir(first_item));
  }
}

void ImagesSelectorDialog::sortItems(QList<QString>& list) {
  QCollator collator;
  collator.setNumericMode(true);
  std::sort(list.begin(), list.end(), collator);
}

void ImagesSelectorDialog::convertPathsToAbsolute(QDir dir, QList<QString>& list) {
  for (QString& path : list) {
    path = dir.absoluteFilePath(path);
  }
}

void ImagesSelectorDialog::keyPressEvent(QKeyEvent* pe) {
  if (pe->key() & Qt::Key_Escape && pe->modifiers() & Qt::NoModifier) {
    reject();
  }
  QDialog::keyPressEvent(pe);
}