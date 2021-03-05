#include "DownloadingDialog.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QRadioButton>
#include <QPushButton>
#include <QDir>
#include <QButtonGroup>
#include <QHBoxLayout>

QHBoxLayout* DownloadingDialog::setUpButtons()
{
  QHBoxLayout* buttonLayout = new QHBoxLayout;
  buttonLayout->setSizeConstraint(QLayout::SetMinimumSize);
  buttonLayout->setObjectName("button_layout");
  QButtonGroup* pbg = new QButtonGroup(this);

  QRadioButton* button = new QRadioButton(".jpg");
  formatButtons.push_back(button);
  button->setChecked(true);
  buttonLayout->addWidget(button);
  pbg->addButton(button);
  connect(button, &QRadioButton::clicked, [&] {
      emit formatIsChanged(".jpg");
  });

  button = new QRadioButton(".png");
  formatButtons.push_back(button);
  buttonLayout->addWidget(button);
  pbg->addButton(button);
  connect(button, &QRadioButton::clicked, [&] {
      emit formatIsChanged(".png");
  });
  return buttonLayout;
}

QHBoxLayout* DownloadingDialog::setUpNumbers()
{
  current = new CurrentLoad;
  inTotal = new LoadsInTotal;
  current->prepareWidgets();
  inTotal->prepareWidgets();
  inTotal->setAlignment(Qt::AlignRight);

  QHBoxLayout* numLayout = new QHBoxLayout;
  numLayout->setObjectName("number_layout");
  numLayout->addLayout(current);
  numLayout->addWidget(new QLabel("/"), 0, Qt::AlignCenter);
  numLayout->addLayout(inTotal);
  return numLayout;
}

void DownloadingDialog::construct(QGridLayout* mainLayout)
{
  mainLayout->setObjectName("main_layout");
  mainLayout->setColumnStretch(0, 5);

  QLabel* linkLabel = new QLabel("&Link:");
  link = new ImageLink(linkLabel);
  link->prepareWidgets();

  start = new LoadButton;
  start->prepareWidgets();

  folderLink = new QLabel;
  folderLink->setText("here will be displayed destination folder");

  mainLayout->addWidget(new QLabel("Folder Name:"), 0, 0, 1, 1);
  mainLayout->addWidget(new QLabel("Format:"),      0, 1, 1, 1, Qt::AlignCenter);

  mainLayout->addWidget(folderLink, 1, 0, 1, 1);
  mainLayout->addLayout(setUpButtons(), 1, 1, 1, 1, Qt::AlignCenter);

  mainLayout->addWidget(linkLabel,     2, 0, 1, 1);
  mainLayout->addWidget(new QLabel("Quantity:"), 2, 1, 1, 1);

  mainLayout->addLayout(link,           3, 0, 1, 1);
  mainLayout->addLayout(setUpNumbers(), 3, 1, 1, 1, Qt::AlignCenter);

  mainLayout->addLayout(start, 4, 0, 1, -1);
  
  setLayout(mainLayout);
}

void DownloadingDialog::enableButtons(bool cond)
{
  for (QRadioButton* iter : formatButtons) {
    iter->setEnabled(cond);
  }
}

void DownloadingDialog::createConnections()
{
  connect(current, &CurrentLoad::valueIsPrepared, this, &DownloadingDialog::currentIsUpdated);
  connect(inTotal, &LoadsInTotal::valueIsPrepared, this, &DownloadingDialog::totalIsUpdated);
  connect(link, &ImageLink::valueIsPrepared, this, &DownloadingDialog::linkIsReady);
  
  connect(start, &LoadButton::buttonClicked, [&] {
    bool c = current->displayValue();
    bool t = inTotal->displayValue();
    bool l = link->displayValue();
    if (c && t && l) {
      start->setRange(current->value(), inTotal->value());
      enableButtons(false);
      start->switchTo<QProgressBar>();
      emit startDownloading();
    }
  });

  connect(start, &LoadButton::startDownloading, this, &DownloadingDialog::startDownloading);
}

DownloadingDialog::DownloadingDialog(QWidget* pwgt)
    : QDialog(pwgt, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
  
}

void DownloadingDialog::prepareDialog() {
  construct(new QGridLayout);
  int h = layout()->minimumSize().height();
  setFixedSize(600, h);
  createConnections();
}

void DownloadingDialog::setProgressValue(int value) {
  start->step();
  current->setValue(value);
}

void DownloadingDialog::upgradeLink()
{
  enableButtons(true);
  start->upgradeLink();
  current->editValue();
  inTotal->editValue();
  link->editValue();
}

void DownloadingDialog::resetDialog()
{
  enableButtons(true);
  current->resetWidgets();
  inTotal->resetWidgets();
  link->resetWidgets();
  start->resetWidgets();
  folderLink->setText("here will be displayed destination folder");
  emit dialogIsReset();
}

void DownloadingDialog::mouseDoubleClickEvent(QMouseEvent* pe) {
  resetDialog();
  QDialog::mouseDoubleClickEvent(pe);
}

void DownloadingDialog::displaySuccess() {
  start->displaySuccess();
}

void DownloadingDialog::setDestinationFolder(const QString& path)
{
  QString ref = "<a href=" + path + ">" + path + "</a>";
  folderLink->setText(ref);
  folderLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
  folderLink->setOpenExternalLinks(true);
}