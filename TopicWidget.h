#pragma once

#include <QDir>
#include <QToolBox>

namespace Ui {
class TopicWidget;
}

/**
 * @brief The TopicWidget class The single widget used to display the files of a single topic
 */
class TopicWidget : public QWidget
{
  Q_OBJECT

public:

  /**
   * @brief TopicWidget Constructor with the folder and file template for loading and creating files
   * @param topicDir
   * @param fileTemplate
   * @param dateTimeFormat
   * @param editable
   * @param parent
   */
  explicit TopicWidget(const QDir &topicDir,
                       const QString &fileTemplate,
                       const QString &dateTimeFormat,
                       const bool &editable = false,
                       QToolBox *parent = nullptr);

  /**
   * @brief ~TopicWidget Destructor
   */
  virtual ~TopicWidget();

  /**
   * @brief init Clear selection set focus away from list view
   */
  void init();

  /**
   * @brief setIndex Select a specific file from the list of available ones
   * @param index
   */
  void setIndex(int index);

signals:

  /**
   * @brief fileSelected Emitted when a new file is selected by double clicking an entry in the list view
   * @param fullPath
   */
  void fileSelected(const QString &fullPath);

private slots:

  /**
   * @brief on_pushButtonEditName_clicked Rename the topic
   */
  void on_pushButtonEditName_clicked();

  /**
   * @brief on_pushButtonSaveName_clicked Save changed topic
   */
  void on_pushButtonSaveName_clicked();

  /**
   * @brief on_listViewNotes_clicked New file selected
   * @param index
   */
  void on_listViewNotes_clicked(const QModelIndex &index);

  /**
   * @brief on_toolButtonAddNote_clicked Create new file
   */
  void on_toolButtonAddNote_clicked();

private:

  /**
   * @brief createNewFileName
   * @return The new current file name based on the given template
   */
  QString createNewFileName();

  Ui::TopicWidget *ui;

  /**
   * @brief m_TopicDir Which topic directory to show
   */
  QDir m_TopicDir;

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
   * @brief m_Editable
   */
  bool m_Editable;

  /**
   * @brief m_FileNames All available topic files
   */
  QStringList m_FileNameModel;

  /**
   * @brief m_ToolBox The assigned toolbox
   */
  QToolBox* m_ToolBox;

  /**
   * @brief m_Index This is our index to be used to update the caption text
   */
  int m_Index;
};
