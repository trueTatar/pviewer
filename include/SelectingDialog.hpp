#ifndef SELECTINGDIALOG_HPP
#define SELECTINGDIALOG_HPP

#include <QDialog>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>

#include <QDebug>
#include "lists.h"
#include "move.h"

#include "global_path.hpp"

class QString;
class QStringList;
class QKeyEvent;

class Mode {
private:
  inline static QCheckBox* check = nullptr;
public:
  static QCheckBox* checkBox() {
    if (check == nullptr) {
      check = new QCheckBox("&Clipboard");
    }
    return check;
  }
protected:
  virtual QDir overrided() = 0;
public:
  virtual ~Mode() = default;
  QDir dir() {
    if (check->isChecked()) {
      check->setChecked(false);
      return QDir( QApplication::clipboard()->text() );
    } else {
      return overrided();
    }
  }
};

class Manual : public Mode {
private:
  QObject* parent;
  QWidget* widget() { return dynamic_cast<QWidget*>(parent); }
protected:
  QDir overrided() override {
    QString s = "Select directory";
    return QDir( QFileDialog::getExistingDirectory( widget(), s, g_basicPath ) );
  }
};

class FromParameter : public Mode {
private:
  QDir parameter_;
public:
  FromParameter(const QDir& p) : parameter_(p) { }
protected:
  QDir overrided() override {
    return parameter_;
  }
};

class SelectingDialog : public QDialog {
  Q_OBJECT
private:
  void convertPathsToAbsolute(QDir, QList<QString>&);
  void sortItems(QList<QString>&);
  QPushButton* createButton(QString);
  QPushButton* createButton(QString, void(SelectingDialog::*)());

  void slotFiles();
  void slotDownload();
  void slotEmbedded(std::unique_ptr<Mode>);
  void slotDirectory(std::unique_ptr<Mode>);
protected:
  void keyPressEvent(QKeyEvent*) override;
public:
  SelectingDialog(QWidget* = nullptr);
  void AssociateWith(std::shared_ptr<FolderPath> p) {
    connect(p.get(), &FolderPath::folderIsChanged, [this] (QDir dir) {
      slotDirectory(std::make_unique<FromParameter>(dir));
    });
  }
  void setDirectory(QString);
  void setImages(QList<QString>);
signals:
  void updateFolderList(QStringList);
  void stringListPrepared(QList<QString>);
  void downloadChosen();
};


#endif // SELECTINGDIALOG_HPP