#include <QPushButton>
#include <QGridLayout>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QCollator>
#include <QTimer>
#include <QKeyEvent>

#include "SelectingDialog.hpp"

QPushButton* SelectingDialog::createButton(QString name, void(SelectingDialog::*pointer)()) {
  QPushButton* button = new QPushButton(name);
  connect(button, &QPushButton::clicked, this, pointer);
  return button;
}

QPushButton* SelectingDialog::createButton(QString name) {
  QPushButton* button = new QPushButton(name);
  connect(button, &QPushButton::clicked, [&] {
    slotEmbedded(std::make_unique<Manual>());
  });
  return button;
}

SelectingDialog::SelectingDialog(QWidget* pwgt)
    : QDialog(pwgt, Qt::WindowCloseButtonHint) {
  QGridLayout* buttonLayout = new QGridLayout( this );
  buttonLayout->addWidget( Mode::checkBox(), 1, 0, 1, -1 );
  buttonLayout->addWidget( createButton("&File(s)", &SelectingDialog::slotFiles), 0, 0 );
  buttonLayout->addWidget( createButton("&Directory"), 0, 1 );
  buttonLayout->addWidget( createButton("&Embedded"),  0, 2 );
  buttonLayout->addWidget( createButton("Down&load", &SelectingDialog::slotDownload), 0, 3 );
}

void SelectingDialog::setDirectory(QString path) {
  if (!path.isEmpty()) {
    slotDirectory(std::make_unique<FromParameter>(path));
  }
}

void SelectingDialog::setImages(QList<QString> images) {
  emit stringListPrepared(images);
}

void SelectingDialog::slotDownload() {
  accept();
  emit downloadChosen();
}

void SelectingDialog::slotFiles() {
  accept();
  QList<QString> pixmap_list = QFileDialog::getOpenFileNames(
      dynamic_cast<QWidget*>(parent()), "Select one or more files to open",
      g_basicPath, "Images (*.png *.jpg *.jpeg)");
  emit stringListPrepared(pixmap_list);
}

void SelectingDialog::slotDirectory(std::unique_ptr<Mode> mode) {
  accept();
  QDir directory = mode->dir();
  directory.setNameFilters(QStringList() << "*.jpg" << "*.jpeg" << "*.png");
  QList<QString> pixmap_list = directory.entryList( QDir::Files );
  sortItems(pixmap_list);
  convertPathsToAbsolute(directory, pixmap_list);
  emit stringListPrepared(pixmap_list);
}

void SelectingDialog::slotEmbedded(std::unique_ptr<Mode> mode)
{
  accept();
  QDir directory = mode->dir();
  QStringList dir_list = directory.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
  if (dir_list.empty()) {
    slotDirectory(std::make_unique<FromParameter>(directory));
  }
  else {
    convertPathsToAbsolute(directory, dir_list);
    emit updateFolderList(dir_list);
    QString first_item = dir_list.at(0);
    slotDirectory(std::make_unique<FromParameter>(QDir(first_item)));
  }
}

void SelectingDialog::sortItems(QList<QString>& list)
{
  QCollator collator;
  collator.setNumericMode(true);
  std::sort(list.begin(), list.end(), collator);
}

void SelectingDialog::convertPathsToAbsolute(QDir dir, QList<QString>& list)
{
  for (QString& path : list) {
    path = dir.absoluteFilePath(path);
  }
}

void SelectingDialog::keyPressEvent( QKeyEvent* pe )
{
  if (pe->key() & Qt::Key_Escape && pe->modifiers() & Qt::NoModifier) {
    reject();
  }
  QDialog::keyPressEvent( pe );
}