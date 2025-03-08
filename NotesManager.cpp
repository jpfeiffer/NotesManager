#include "NotesManager.h"
#include "ui_NotesManager.h"

#include <QToolBox>
#include <QSettings>
#include <QProcess>
#include <QSaveFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QCryptographicHash>

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

#include "TopicWidget.h"

namespace
{

static const QString cEnergyAcOnline("/sys/class/power_supply/AC/online");
static const QString cEnergyNowTemplate = QString("/sys/class/power_supply/BAT%1/energy_now");
static const QString cEnergyFullTemplate = QString("/sys/class/power_supply/BAT%1/energy_full");

/**
 * @brief cPeriodicIntervalMs Periodic refresh every 500ms
 */
static const int cPeriodicIntervalMs = 500;

/**
 * @brief cLockTimeoutIntervalMs 10 Minutes lock interval
 */
static const int cLockTimeoutIntervalMs = 10 * 60 * 1000;

/**
 * @brief cAutomaticSaveIntervalMs Every 2 seconds after last edit we save
 */
static const int cLockAutoSaveIntervalMs = 2 * 1000;

QList<QPair<QFileInfo,QFileInfo>> QueryBackupFiles(const QDir &sourceDir, const QDir &destinationDir)
{
  QList<QPair<QFileInfo,QFileInfo>> filesToBackup;

  QList<QFileInfo> files = sourceDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
  for(const auto &fileInfo : files)
  {
    if(fileInfo == QFileInfo(sourceDir.absolutePath())) continue;

    if(true == fileInfo.isDir())
    {
      QString nestedDestinationDir = destinationDir.absoluteFilePath(fileInfo.fileName());
      filesToBackup << QueryBackupFiles(QDir(fileInfo.absoluteFilePath()), QDir(nestedDestinationDir));
    }
    else if(fileInfo.isFile())
    {
      filesToBackup << qMakePair(fileInfo, QFileInfo(destinationDir.absoluteFilePath(fileInfo.fileName())));
    }
  }

  return filesToBackup;
}
//----------------------------------------------------------------------------------------------------------------------

}

NotesManager::BatteryStatus::BatteryStatus(int i)
  : index(i)
  , energyNow(cEnergyNowTemplate.arg(index))
  , energyFull(cEnergyFullTemplate.arg(index))
{
}
//----------------------------------------------------------------------------------------------------------------------

NotesManager::BatteryStatus::BatteryStatus(const BatteryStatus &other)
  : BatteryStatus(other.index)
{
}
//----------------------------------------------------------------------------------------------------------------------

bool NotesManager::BatteryStatus::exists() const
{
  return energyNow.exists() && energyFull.exists();
}
//----------------------------------------------------------------------------------------------------------------------

int NotesManager::BatteryStatus::now()
{
  energyNow.open(QIODevice::ReadOnly | QIODevice::Text);
  auto now = QString(energyNow.readAll()).toInt();
  energyNow.close();

  return now;
}
//----------------------------------------------------------------------------------------------------------------------

int NotesManager::BatteryStatus::full()
{
  energyFull.open(QIODevice::ReadOnly | QIODevice::Text);
  auto full = QString(energyFull.readAll()).toInt();
  energyFull.close();

  return full;
}
//----------------------------------------------------------------------------------------------------------------------

NotesManager::NotesManager(const NotesManagerSettings &settings,
                           QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::NotesManager)
  , m_Settings(settings)
  , m_BatteryStatus(new QLabel(this))
  , m_PeriodicTimer(new QTimer(this))
  , m_LockTimer(new QTimer(this))
  , m_ToolBox(new QToolBox(this))
  , m_CurrentFilePath()
  , m_LastFileSave()
  , m_QUdev(new QUdev())
  , m_Watcher()
  , m_StorageInfo()
{
  qApp->installEventFilter(this);

  ui->setupUi(this);
  ui->stackedWidget->setCurrentWidget(ui->pageLogin);
  ui->verticalLayoutTopics->addWidget(m_ToolBox);
  ui->pushButtonAddTopic->setVisible(m_Settings.m_Editable);

  ui->pushButtonSizeNormal->setProperty("fontSize", QVariant::fromValue<int>(m_Settings.m_NormalSize));
  ui->pushButtonSizeLarge->setProperty("fontSize", QVariant::fromValue<int>(m_Settings.m_LargeSize));
  ui->pushButtonSizeHuge->setProperty("fontSize", QVariant::fromValue<int>(m_Settings.m_HugeSize));

  auto font = ui->plainTextEdit->font();
  font.setPointSize(m_Settings.m_NormalSize);
  ui->plainTextEdit->setFont(font);

  ui->statusbar->addPermanentWidget(m_BatteryStatus);
  m_BatteryStatus->setAlignment(Qt::AlignRight);

  m_PeriodicTimer->setInterval(cPeriodicIntervalMs);
  m_LockTimer->setInterval(cLockTimeoutIntervalMs);

  connect(m_PeriodicTimer, &QTimer::timeout, this, &NotesManager::onPeriodicTimer);
  connect(m_LockTimer, &QTimer::timeout, this, &NotesManager::onLockTimeout);
  connect(ui->lineEditPassCode, &QLineEdit::textChanged, this, &NotesManager::onPassCodeChanged);
  connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, &NotesManager::onContentChanged);
  connect(ui->pushButtonAddTopic, &QPushButton::clicked, this, &NotesManager::onAddTopicButtonClicked);
  connect(m_ToolBox, &QToolBox::currentChanged, this, &NotesManager::onCurrentTopicIndexChanged);

  connect(ui->pushButtonSizeNormal, &QPushButton::clicked, this, &NotesManager::onFontSizeButtonClicked);
  connect(ui->pushButtonSizeLarge, &QPushButton::clicked, this, &NotesManager::onFontSizeButtonClicked);
  connect(ui->pushButtonSizeHuge, &QPushButton::clicked, this, &NotesManager::onFontSizeButtonClicked);

  connect(&m_Watcher, &QFutureWatcher<int>::finished, this,
          [this]()
  {
    ui->statusbar->showMessage(tr("Backup to USB complete"), 5000);
    QProcess::execute("/usr/bin/udiskie-umount", {m_StorageInfo.device()});
  });

  for(const auto &topic : m_Settings.m_TopicNames)
  {
    if(false == m_Settings.m_BaseDirectory.exists(topic)) m_Settings.m_BaseDirectory.mkpath(topic);

    if(true == m_Settings.m_BaseDirectory.exists(topic))
    {
      auto dir = m_Settings.m_BaseDirectory;
      dir.cd(topic);

      auto topicWidget = new TopicWidget(dir,
                                         m_Settings.m_FileTemplate,
                                         m_Settings.m_DateTimeFormat,
                                         m_Settings.m_Editable, m_ToolBox);
      auto index = m_ToolBox->addItem(topicWidget, topic);
      topicWidget->setIndex(index);
      connect(topicWidget, &TopicWidget::fileSelected, this, &NotesManager::onFileSelected);
    }
  }

  connect(m_QUdev.get(), &QUdev::newUDevEvent, this, &NotesManager::onNewUdevEvent);
  m_QUdev->addNewMonitorRule(QString("block"), QString("partition"), QString("usb"), QString("usb_device"));
  m_QUdev->addNewMonitorRule(QString("block"), QString("disk"), QString("usb"), QString("usb_device"));

  refreshBatteryStatus();

  //editing requires a selected file
  ui->plainTextEdit->setEnabled(false);
  //start periodic updates
  m_PeriodicTimer->start();
}
//----------------------------------------------------------------------------------------------------------------------

NotesManager::~NotesManager()
{
  delete ui;

  m_BatteryStatus->deleteLater();
  m_ToolBox->deleteLater();
  m_PeriodicTimer->deleteLater();
  m_LockTimer->deleteLater();
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onNewUdevEvent(QUdevEvent event)
{
  auto devPath = event.m_udDev.m_strDevPath;

  if(QUdevEventAction::eDeviceAdd == event.m_ueAction)
  {
    auto storageInfos = QStorageInfo::mountedVolumes();
    for(const auto &storageInfo : storageInfos)
    {
      const auto devicePathFromStorageInfo = QString::fromLatin1(storageInfo.device());
      if(devicePathFromStorageInfo == devPath)
      {
        m_StorageInfo = storageInfo;
        backupAllFilesToDirectory(m_StorageInfo.rootPath());
        break;
      }
    }
  }
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onFileSelected(const QString &fileName)
{
  saveCurrentContent();

  if(m_CurrentFilePath != fileName)
  {
    ui->plainTextEdit->clear();
    m_CurrentFilePath = fileName;

    bool opened{};

    if(false == m_CurrentFilePath.isEmpty())
    {
      QFile selectedFile(m_CurrentFilePath);
      opened = selectedFile.open(QIODevice::ReadWrite);

      if(true == opened)
      {
        QTextStream ts(&selectedFile);
        ui->plainTextEdit->setPlainText(ts.readAll());
      }
    }

    ui->plainTextEdit->setEnabled(opened);
  }
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onFontSizeButtonClicked()
{
  auto button = dynamic_cast<QPushButton*>(sender());
  if(nullptr == button) return;

  auto size = button->property("fontSize").toInt();

  auto font = ui->plainTextEdit->font();
  font.setPointSize(size);
  ui->plainTextEdit->setFont(font);
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onLockTimeout()
{
  m_LockTimer->stop();
  m_LastFileSave.invalidate();

  ui->stackedWidget->setCurrentWidget(ui->pageLogin);
  ui->lineEditPassCode->clear();
  ui->lineEditPassCode->setFocus();
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onPeriodicTimer()
{
  if((true == m_LastFileSave.isValid()) && (cLockAutoSaveIntervalMs < m_LastFileSave.elapsed()))
  {
    saveCurrentContent();
  }

  refreshBatteryStatus();
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onContentChanged()
{
  m_LastFileSave.restart();
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onAddTopicButtonClicked()
{
  const auto defaultName = tr("Topic");
  auto dir = m_Settings.m_BaseDirectory;

  if(true == dir.mkpath(defaultName))
  {
    dir.cd(defaultName);

    auto topicWidget = new TopicWidget(dir,
                                       m_Settings.m_FileTemplate,
                                       m_Settings.m_DateTimeFormat,
                                       m_Settings.m_Editable,
                                       m_ToolBox);
    auto index = m_ToolBox->addItem(topicWidget, defaultName);
    topicWidget->setIndex(index);
  }
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::saveCurrentContent()
{
  if(false == m_CurrentFilePath.isEmpty())
  {
    const auto saved = saveContentToFile(m_CurrentFilePath);

    const auto fileName = QFileInfo(m_CurrentFilePath).fileName();
    ui->statusbar->showMessage(saved ? tr("Saved: %1").arg(fileName)
                                     : tr("Failed to save: %1").arg(fileName), 5000);

    m_LastFileSave.invalidate();
  }
}
//----------------------------------------------------------------------------------------------------------------------

bool NotesManager::saveContentToFile(const QString &file) const
{
  if(true == file.isEmpty()) return false;

  QSaveFile saveFile(file);
  if(true == saveFile.open(QIODevice::WriteOnly))
  {
    QTextStream ts(&saveFile);
    ts << ui->plainTextEdit->toPlainText();
  }

  return saveFile.commit();
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::refreshBatteryStatus()
{
  //per default we are running on mains
  bool ac = true;
  int batteryIndex{};
  int batteryLevel{};
  int sumNow{};
  int sumFull{};

  QFile acLine(cEnergyAcOnline);

  if(acLine.exists())
  {
      acLine.open(QIODevice::ReadOnly | QIODevice::Text);
      ac = QString(acLine.readAll()).toInt() ? true : false;
      acLine.close();
  }

  while(true)
  {
     BatteryStatus status(batteryIndex++);
     if(false == status.exists()) break;

     sumNow += status.now();
     sumFull += status.full();
  }

  batteryLevel = qRound(100.0 * double(sumNow) / double(sumFull));
  m_BatteryStatus->setText(ac ? tr("Mains power") : tr("Battery powered (%1%)").arg(batteryLevel));
}
//----------------------------------------------------------------------------------------------------------------------

bool NotesManager::backupAllFilesToDirectory(const QString &targetDirectory)
{
  const auto backupDateFormat = QString("yyyy-MM-dd");
  const auto dateTime = QDateTime::currentDateTime();
  const QString dateTimeString = dateTime.toString(backupDateFormat);

  const auto backupDirName = tr("Backup Notes - %1").arg(dateTimeString);
  const auto backupDestination = QDir(targetDirectory).absoluteFilePath(backupDirName);

  auto files = QueryBackupFiles(m_Settings.m_BaseDirectory, backupDestination);

  auto copyFile = [this](const QPair<QFileInfo, QFileInfo> &info) -> int
  {
    if(false == info.first.exists()) return 0;

    auto destinationAvailable = QDir().mkpath(info.second.absolutePath());
    if(false == destinationAvailable) return 0;

    auto destinationName = info.second.absoluteFilePath().replace(':', '-');
    if(true == QFileInfo(destinationName).exists()) QFile::remove(destinationName);

    return QFile::copy(info.first.absoluteFilePath(), destinationName) ? 1 : 0;
  };

  QFuture<int> copies = QtConcurrent::mapped(files, copyFile);
  m_Watcher.setFuture(copies);
  return true;
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onPassCodeChanged(const QString &passcode)
{
  if(ui->pageLogin != ui->stackedWidget->currentWidget()) return;

  const auto hashBytes = QString("%1%2").arg(passcode, qApp->applicationName()).toLatin1();
  const auto hash = QCryptographicHash::hash(hashBytes, QCryptographicHash::Sha256).toHex();

  if(m_Settings.m_UnlockPinHash == hash)
  {
    ui->stackedWidget->setCurrentWidget(ui->pageNotes);
    m_LockTimer->start();
  }
}
//----------------------------------------------------------------------------------------------------------------------

void NotesManager::onCurrentTopicIndexChanged(int index)
{
  TopicWidget* topicWidget = qobject_cast<TopicWidget*>(m_ToolBox->currentWidget());
  if(nullptr == topicWidget) return;

  topicWidget->init();

  saveCurrentContent();

  ui->plainTextEdit->setEnabled(false);
  ui->plainTextEdit->clear();
  m_CurrentFilePath = "";
}
//----------------------------------------------------------------------------------------------------------------------

bool NotesManager::eventFilter(QObject *watched, QEvent *event)
{
  if(nullptr == event) return false;

  if((QEvent::MouseMove == event->type()) || (QEvent::KeyPress == event->type()))
  {
    if(true == m_LockTimer->isActive()) m_LockTimer->start();
  }

  return false;
}
//----------------------------------------------------------------------------------------------------------------------
