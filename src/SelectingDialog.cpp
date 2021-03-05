#include <QPushButton>
#include <QGridLayout>
#include <QCheckBox>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QCollator>
#include <QTimer>
#include <QKeyEvent>

#include "SelectingDialog.hpp"

int FolderA::index_ = 0;
QStringList FolderA::list_;
QCheckBox* Mode::check = nullptr;
bool FolderA::possibleToMoveIndex() {
  return !list_.empty();
}

QPushButton* SelectingDialog::createButton(QString name, void(SelectingDialog::*pointer)())
{
  QPushButton* button = new QPushButton(name);
  connect(button, &QPushButton::clicked, this, pointer);
  return button;
}

QPushButton* SelectingDialog::createButton(QString name)
{
  QPushButton* button = new QPushButton(name);
  connect(button, &QPushButton::clicked, [&] {
    slotEmbedded(std::make_unique<Manual>());
  });
  return button;
}

SelectingDialog::SelectingDialog(QWidget* pwgt)
  : QDialog( pwgt, Qt::WindowCloseButtonHint )
{
  createFolderNameMB();
  QGridLayout* buttonLayout = new QGridLayout( this );
  buttonLayout->addWidget( Mode::checkBox(), 1, 0, 1, -1 );
  buttonLayout->addWidget( createButton("&File(s)", &SelectingDialog::slotFiles), 0, 0 );
  buttonLayout->addWidget( createButton("&Directory"), 0, 1 );
  buttonLayout->addWidget( createButton("&Embedded"),  0, 2 );
  buttonLayout->addWidget( createButton("Down&load", &SelectingDialog::slotDownload), 0, 3 );
}

void SelectingDialog::setDirectory(QString path)
{
  if (!path.isEmpty()) {
    slotDirectory(std::make_unique<FromParameter>(path));
  }
}

void SelectingDialog::slotDownload()
{
  accept();
  emit downloadChosen();
}

void SelectingDialog::slotFiles()
{
  accept();
  pixmapList = QFileDialog::getOpenFileNames( dynamic_cast<QWidget*>( parent() ),
    "Select one or more files to open",
    g_basicPath,
    "Images (*.png *.jpg *.jpeg)" );
  emit stringListPrepared( pixmapList );
}

void SelectingDialog::slotDirectory(std::unique_ptr<Mode> mode)
{
  accept();
  QDir directory = mode->dir();
  directory.setNameFilters(QStringList() << "*.jpg" << "*.jpeg" << "*.png");
  pixmapList = directory.entryList( QDir::Files );
  sortItems(pixmapList);
  convertPathsToAbsolute(directory, pixmapList);

  emit stringListPrepared( pixmapList );
}

void SelectingDialog::slotEmbedded(std::unique_ptr<Mode> mode)
{
  accept();
  QDir directory = mode->dir();
  QStringList dirList = directory.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
  if (dirList.empty()) {
    slotDirectory( std::make_unique<FromParameter>(directory) );
  }
  else {
    convertPathsToAbsolute(directory, dirList);
    FolderA::setFolder(dirList);
    slotDirectory( std::make_unique<FromParameter>(FolderA::folder()) );
  }
}

void SelectingDialog::sortItems(QStringList& list)
{
  QCollator collator;
  collator.setNumericMode( true );
  std::sort( list.begin(), list.end(), collator );
}

void SelectingDialog::convertPathsToAbsolute(QDir dir, QStringList& list)
{
  for (QString& path : list) {
    path = dir.absoluteFilePath( path );
  }
}

void SelectingDialog::createFolderNameMB()
{
  messageBox = new QMessageBox( dynamic_cast<QWidget*>( parent() ) );
  messageBox->setIcon( QMessageBox::NoIcon );
}

void SelectingDialog::display(QString info, int forTime)
{
  messageBox->setText(info);
  QTimer::singleShot( forTime, messageBox, &QMessageBox::accept );
  messageBox->exec();
}

void SelectingDialog::goTo(std::unique_ptr<FolderA> p)
{
  if (p->possibleToMoveIndex()) {
    p->moveIndex();
    QDir dir = p->folder();
    slotDirectory( std::make_unique<FromParameter>(dir) );
    display(dir.dirName(), 600);
    emit folderIsChanged();
  } else {
    display(p->errorMessage(), 1500);
  }
}

void SelectingDialog::keyPressEvent( QKeyEvent* pe )
{
  if (pe->key() & Qt::Key_Escape && pe->modifiers() & Qt::NoModifier) {
    reject();
  }
  QDialog::keyPressEvent( pe );
}