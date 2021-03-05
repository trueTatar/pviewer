#ifndef SELECTINGDIALOG_HPP
#define SELECTINGDIALOG_HPP

#include <QDialog>
#include <QDir>

#include <QtWidgets>

#include "global_path.hpp"

class QString;
class QStringList;
class QKeyEvent;

class Mode {
private:
  static QCheckBox* check;
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
  const QString g_basicPath = "/home/user/Pictures/";
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

class FolderA {
protected:
  static int index_;
  static QStringList list_;
public:
  static void setFolder(const QStringList& l) {
    list_ = l;
  }
  static QDir folder() {
    return list_.at(index_);
  }
  virtual void moveIndex() = 0;
  virtual bool possibleToMoveIndex() = 0;
  virtual QString errorMessage() = 0;
};

class NextFolder : public FolderA {
public:
  bool possibleToMoveIndex() override {
    return FolderA::possibleToMoveIndex() && (index_ != list_.size() - 1);
  }
  void moveIndex() override { ++index_; }
  QString errorMessage() override { return "there is no next folder"; }
};

class PreviousFolder : public FolderA {
public:
  bool possibleToMoveIndex() override {
    return FolderA::possibleToMoveIndex() && (index_ != 0);
  }
  void moveIndex() override { --index_; }
  QString errorMessage() override { return "there is no previous folder"; }
};

class SelectingDialog : public QDialog {
  Q_OBJECT
private:
  QStringList pixmapList;
  QMessageBox* messageBox;

  void display(QString, int);
  void createFolderNameMB();
  void convertPathsToAbsolute(QDir, QStringList&);
  void sortItems(QStringList&);

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
  void goTo(std::unique_ptr<FolderA>);
  void setDirectory(QString);
signals:
  void folderIsChanged();
  void stringListPrepared(QStringList);
  void downloadChosen();
};

#endif // SELECTINGDIALOG_HPP