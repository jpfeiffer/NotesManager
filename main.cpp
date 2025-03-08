#include "NotesManager.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

static const QString cSettingsFile = QString("topics.ini");
static const QString cDefaultPin = QString("030910");
static const int cDefaultNormalSize = 11;
static const int cDefaultLargeSize = 14;
static const int cDefaultHugeSize = 17;

static const QStringList cDefaultTopicNames = {"Mathematik",
                                               "Deutsch",
                                               "Geschichte",
                                               "Geographie",
                                               "Englisch",
                                               "Biologie",
                                               "Physik",
                                               "Informatik",
                                               "Religion",
                                               "GKR",
                                               "Chemie",
                                               "Musik",
                                               "Französisch",
                                               "WPA",
                                               "WTH"};

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  QTranslator translator;
  NotesManagerSettings settings;

  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages)
  {
    const QString baseName = "NotesManager_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName))
    {
      a.installTranslator(&translator);
      break;
    }
  }

  int normalSize = cDefaultNormalSize;
  int largeSize = cDefaultLargeSize;
  int hugeSize = cDefaultHugeSize;
  auto fileTemplate = QString("%N - %D");
  auto dtFormat = QString("yyyy-MM-dd hh:mm:ss");
  auto defaultHashInput = QString("%1%2").arg(cDefaultPin, qApp->applicationName());

  QString unlockPinHash(QCryptographicHash::hash(defaultHashInput.toLatin1(), QCryptographicHash::Sha256).toHex());
  auto documentsDirectoryPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  QDir baseDirectory = QDir(documentsDirectoryPath);
  baseDirectory.mkpath("notes");
  baseDirectory.cd("notes");

  QStringList defaultTopicNames = cDefaultTopicNames;

  //check topic names within the optional topic file to generate them if needed
  if(true == baseDirectory.exists(cSettingsFile))
  {
    const auto fileName = baseDirectory.absoluteFilePath(cSettingsFile);
    auto settingsFile = QSettings(fileName, QSettings::IniFormat);

    {
      settingsFile.beginGroup("Topics");
      auto topicNames = settingsFile.value("Names").toStringList();

      if(true == settingsFile.contains("Pin")) unlockPinHash = settingsFile.value("Pin").toString().toLatin1();
      if(true == settingsFile.contains("FileTemplate")) fileTemplate = settingsFile.value("FileTemplate").toString();
      if(true == settingsFile.contains("DateTimeFormat")) dtFormat = settingsFile.value("DateTimeFormat").toString();

      if(true == settingsFile.contains("NormalSize")) normalSize = settingsFile.value("NormalSize").toInt();
      if(true == settingsFile.contains("LargeSize")) largeSize = settingsFile.value("LargeSize").toInt();
      if(true == settingsFile.contains("HugeSize")) hugeSize = settingsFile.value("HugeSize").toInt();

      settingsFile.endGroup();

      for(const auto &topicName : topicNames)
      {
        if(false == defaultTopicNames.contains(topicName)) defaultTopicNames.append(topicName);
      }
    }
  }

  settings.m_Editable = a.arguments().contains("--editable");
  settings.m_BaseDirectory = baseDirectory;
  settings.m_FileTemplate = fileTemplate;
  settings.m_DateTimeFormat = dtFormat;
  settings.m_UnlockPinHash = unlockPinHash;
  settings.m_TopicNames = defaultTopicNames;
  settings.m_NormalSize = normalSize;
  settings.m_LargeSize = largeSize;
  settings.m_HugeSize = hugeSize;

  NotesManager w(settings);
  w.show();

  return a.exec();
}
