#ifndef DOWNLOADINGDIALOG_HPP
#define DOWNLOADINGDIALOG_HPP

#include <type_traits>

#include <QtWidgets>
#include <QStackedLayout>
#include <QDialog>
class QWidget;
class QLabel;
class QGridLayout;
class QHBoxLayout;
class QString;

class DownloadingDialog;
class CurrentLoad;
class LoadsInTotal;
class Link;

class RestrictedLayout : public QStackedLayout {
public:
  QSize minimumSize() const
  {
    QSize s(0, 0);
    int n = count();
    for (int i = 0; i != n; ++i) {
      QLayoutItem* o = itemAt(i);
      s = s.expandedTo(o->minimumSize());
    }
    return s;
  }
  QSize sizeHint() const override {
    QSize s(0, 0);
    int n = count();
    if (n > 0) {
      //s = QSize(100, 70); //start with a nice default size
      s = QSize(0,0);
    }
    for (int i = 0; i != n; ++i) {
      QLayoutItem *o = itemAt(i);
      s = s.expandedTo(o->sizeHint());
    }
    return s;
  }

  void setGeometry(const QRect &r) override {
    QLayout::setGeometry(r);
    if (count() == 0) { return; }
    for (int i = 0; i != count(); ++i) {
      itemAt(i)->setGeometry(r);
    }
  }
  Qt::Orientations expandingDirections() const override {
    return Qt::Horizontal;
  }
};

template<typename T1, typename T2>
class SwitcherBase : public RestrictedLayout {
private:
  T1* widget1;
  T2* widget2;
protected:
  template<typename T>
  T* get() {
    if constexpr (std::is_same_v<T, T1>) {
      return widget1;
    }
    if constexpr (std::is_same_v<T, T2>) {
      return widget2;
    }
    return nullptr;
  }
  virtual void formatWidgets() = 0;
  virtual void createConnections() = 0;
public:
  SwitcherBase() : widget1(new T1), widget2(new T2) {
    addWidget(widget1);
    addWidget(widget2);
  }
  template<typename T>
  void switchTo() {
    if constexpr (std::is_same_v<T, T1>) {
      setCurrentWidget(widget1);
    }
    if constexpr (std::is_same_v<T, T2>) {
      setCurrentWidget(widget2);
    }
  }
  void prepareWidgets() {
    createConnections();
    formatWidgets();
  }
  virtual void resetWidgets() = 0;
};

class LoadButton : public SwitcherBase<QPushButton, QProgressBar> {
  Q_OBJECT
private:
  int currentValue;
protected:
  void formatWidgets() override {
    int h = 50;
    get<QPushButton>()->setFixedHeight(h);
    get<QProgressBar>()->setFixedHeight(h);
    get<QPushButton>()->setText("&Start");
    get<QProgressBar>()->setFormat("");
  }
  void createConnections() final {
    connect(get<QPushButton>(), &QPushButton::clicked, this, &LoadButton::buttonClicked);
  }
public:
  void setRange(int min, int max) {
    currentValue = 0;
    get<QProgressBar>()->setValue(currentValue);
    get<QProgressBar>()->setMaximum((max + 1) - min);
  }
  void step() {
    get<QProgressBar>()->setValue(++currentValue);
  }
  void upgradeLink() {
    get<QPushButton>()->setText("&Continue");
    switchTo<QPushButton>();
  }
  void displaySuccess() {
    step();
    get<QProgressBar>()->setFormat("Success!");
  }
  void resetWidgets() override {
    get<QProgressBar>()->setFormat("");
    get<QPushButton>()->setText("&Start");
    switchTo<QPushButton>();
  }
signals:
  void startDownloading();
  void buttonClicked();
};

class LEBase : public SwitcherBase<QLineEdit, QLabel> {
  Q_OBJECT
protected:
  void formatWidgets() override {
    get<QLineEdit>()->setAlignment(Qt::AlignCenter);
    get<QLabel>()->setAlignment(Qt::AlignCenter);
    get<QLineEdit>()->setFixedWidth(40);
    get<QLabel>()->setFixedWidth(40);
    get<QLabel>()->setFixedHeight(get<QLineEdit>()->height());
    setDefaultValue();
  }
  void createConnections() final {
    connect(get<QLineEdit>(), &QLineEdit::returnPressed, this, &LEBase::displayValue);
  }
  virtual void setDefaultValue() {
    get<QLineEdit>()->clear();
  }
public:
  void resetWidgets() override {
    setDefaultValue();
    switchTo<QLineEdit>();
  }
  virtual bool displayValue() {
    QString info = get<QLineEdit>()->text();
    bool res = info.toInt();
    if (res) {
      get<QLabel>()->setText(info);
      switchTo<QLabel>();
      emit valueIsPrepared(info);
    }
    return res;
  }
  void editValue() {
    get<QLineEdit>()->setText( get<QLabel>()->text() );
    switchTo<QLineEdit>();
  }
  int value() { return get<QLineEdit>()->text().toInt(); }
signals:
  void valueIsPrepared(QString);
};

class CurrentLoad : public LEBase {
protected:
  void setDefaultValue() override {
    get<QLineEdit>()->setText("1");
  }
public:
  void setValue(int value) {
    get<QLabel>()->setNum(value);
  }
};

class LoadsInTotal : public LEBase {
protected:
  void formatWidgets() override {
    LEBase::formatWidgets();
    get<QLineEdit>()->setLayoutDirection(Qt::RightToLeft);
    get<QLabel>()->setLayoutDirection(Qt::RightToLeft);
  }
};

class ImageLink : public LEBase {
protected:
  void formatWidgets() override {
    get<QLabel>()->setFixedHeight(get<QLineEdit>()->height());
  }
public:
  bool displayValue() override {
    QString info = get<QLineEdit>()->text();
    bool res = !info.isEmpty();
    if (res) {
      get<QLabel>()->setText(info);
      switchTo<QLabel>();
      emit valueIsPrepared(info);
    }
    return res;
  }
public:
  ImageLink(QLabel* lbl) {
    lbl->setBuddy(get<QLineEdit>());
  }
};

class DownloadingDialog : public QDialog {
  Q_OBJECT
private:
  CurrentLoad* current;
  LoadsInTotal* inTotal;
  LoadButton* start;
  ImageLink* link;
  QLabel* folderLink;
  QList<QRadioButton*> formatButtons;

  void construct(QGridLayout*);
  void createConnections();
  QHBoxLayout* setUpButtons();
  QHBoxLayout* setUpNumbers();
  void resetDialog();
  void enableButtons(bool);
protected:
  void mouseDoubleClickEvent( QMouseEvent* ) override;
public:
  DownloadingDialog( QWidget* = nullptr );
  void prepareDialog();
public slots:
  void setProgressValue( int );
  void upgradeLink();
  void setDestinationFolder(const QString&);
  void displaySuccess();

signals:
  void linkIsReady( const QString& );
  void currentIsUpdated( QString );
  void totalIsUpdated( QString );
  void startDownloading();
  void dialogIsReset();
  void formatIsChanged( const QString& );
};

#endif // DOWNLOADINGDIALOG_HPP