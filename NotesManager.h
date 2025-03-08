#pragma once

#include <QDir>
#include <QTimer>
#include <QLabel>
#include <QPointer>
#include <QElapsedTimer>
#include <QMainWindow>

#include "QUdev/QUdev.h"

QT_BEGIN_NAMESPACE
namespace Ui { class NotesManager; }
QT_END_NAMESPACE

class QToolBox;

struct NotesManagerSettings
{
  /**
   * @brief m_Editable Optional flag to enable topic editing
   */
  bool m_Editable;

  /**
   * @brief m_UnlockPin the pin required to unlock the app screen
   */
  QString m_UnlockPinHash;

  /**
   * @brief m_BaseDirectory Here we store all topic directories
   */
  QDir m_BaseDirectory;

  /**
   * @brief m_FileTemplate the template to name the files
   *
   * @note: %N will be replaced with the topic name, %D with the date time string, %C with a simple file counter
   */
  QString m_FileTemplate;

  /**
   * @brief m_DateTimeFormat The date time format to be included in the file name
   */
  QString m_DateTimeFormat;

  /**
   * @brief m_TopicNames Available topics
   */
  QStringList m_TopicNames;

  /**
   * @brief m_NormalSize Normal font size
   */
  int m_NormalSize;

  /**
   * @brief m_LargeSize Larger
   */
  int m_LargeSize;

  /**
   * @brief m_HugeSize Largest font size
   */
  int m_HugeSize;
};

class NotesManager : public QMainWindow
{
  Q_OBJECT

public:
  NotesManager(const NotesManagerSettings &settings,
               QWidget *parent = nullptr);

  ~NotesManager();

private slots:

  /**
   * @brief onNewUDevEvent Handles dynamic detection of new usb partitions
   */
  void onNewUdevEvent(QUdevEvent);

  /**
   * @brief onFileSelected A new file is selected within a topic window
   * @param fileName
   */
  void onFileSelected(const QString &fileName);

  /**
   * @brief onFontSizeButtonClicked The user clicked a button to change the font size
   */
  void onFontSizeButtonClicked();

  /**
   * @brief onLockTimeout Called when the user was inactive for the lock time
   */
  void onLockTimeout();

  /**
   * @brief onPeriodicTimer Gets called when the period timer is activated
   */
  void onPeriodicTimer();

  /**
   * @brief onContentChanged Text changed
   */
  void onContentChanged();

  /**
   * @brief onAddTopicButtonClicked This will create a new topic folder and add the topic to the list of topics
   */
  void onAddTopicButtonClicked();

  /**
   * @brief onPassCodeChanged Callback used to check if the correct passcode has been entered
   * @param passcode
   */
  void onPassCodeChanged(const QString &passcode);

  /**
   * @brief onCurrentTopicIndexChanged
   * @param index The newly selected topic index
   */
  void onCurrentTopicIndexChanged(int index);

private:

  /**
   * @brief The BatteryStatus class is a small helper to read the status and battery value from a single battery
   *
   * Each battery in the system is listed with an index, simply provide the index to query status and values
   */
  struct BatteryStatus
  {
    /**
     * @brief BatteryStatus Create a new instance to the battery with the given index
     * @param i The battery index to check
     */
    BatteryStatus(int i);

    /**
     * @brief BatteryStatus Copy constructor
     * @param other
     */
    BatteryStatus(const BatteryStatus &other);

    /**
     * @brief exists
     * @return True if a battery with the given index is present in the system
     */
    bool exists() const;

    /**
     * @brief now
     * @return Current energy level of the battery (unitless)
     */
    int now();

    /**
     * @brief full
     * @return Maximum energy level of the battery (unitless)
     */
    int full();

    //!Battery index
    int index;
    //!File to read current level
    QFile energyNow;
    //!File to read maximum level
    QFile energyFull;
  };

  /**
   * @brief eventFilter We filter mouse move and keyboard press events to keep track of the user idle time
   * @param watched
   * @param event
   * @return
   */
  virtual bool eventFilter(QObject *watched, QEvent *event) override;

  /**
   * @brief saveCurrentContent Save content from current file and print status in statusbar
   */
  void saveCurrentContent();

  /**
   * @brief saveCurrentContent Use QSaveFile to write current content
   * @param file
   */
  bool saveContentToFile(const QString &file) const;

  /**
   * @brief refreshBatteryStatus
   */
  void refreshBatteryStatus();

  /**
   * @brief backupAllFilesToDirectory Copies all topic files to the target directory with the current timestamp as name
   * @param targetDirectory
   */
  bool backupAllFilesToDirectory(const QString &targetDirectory);

  /**
   * @brief ui The ui elements
   */
  Ui::NotesManager *ui;

  /**
   * @brief m_Settings The global settings object
   */
  NotesManagerSettings m_Settings;

  /**
   * @brief m_BatteryStatus Display battery status
   */
  QLabel* m_BatteryStatus;

  /**
   * @brief m_SaveTimer Used for automatic saves
   */
  QTimer* m_PeriodicTimer;

  /**
   * @brief m_LockTimer Detect timeouts if inactivity
   */
  QTimer* m_LockTimer;

  /**
   * @brief m_ToolBox All topics are listed here
   */
  QToolBox* m_ToolBox;

  /**
   * @brief m_CurrentFilePath Which file to display
   */
  QString m_CurrentFilePath;

  /**
   * @brief m_LastFileSave Periodically save the content
   */
  QElapsedTimer m_LastFileSave;

  /**
   * @brief m_QUdev Instance to observed added/removed USB drives
   */
  std::shared_ptr<QUdev> m_QUdev;

  /**
   * @brief m_Watcher Keep track of copying process for notes
   */
  QFutureWatcher<int> m_Watcher;

  /**
   * @brief m_StorageInfo Where we copy our notes to
   */
  QStorageInfo m_StorageInfo;
};
