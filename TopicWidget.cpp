#include "TopicWidget.h"
#include "qdir.h"
#include "ui_TopicWidget.h"

#include <QDateTime>
#include <QFileSystemModel>
#include <QStringListModel>

TopicWidget::TopicWidget(const QDir &topicDir,
                         const QString &fileTemplate,
                         const QString &dateTimeFormat,
                         const bool &editable,
                         QToolBox *parent)
  : QWidget(parent)
  , ui(new Ui::TopicWidget)
  , m_TopicDir(topicDir)
  , m_FileTemplate(fileTemplate)
  , m_DateTimeFormat(dateTimeFormat)
  , m_Editable(editable)
  , m_ToolBox(parent)
  , m_Index(-1)
{
  ui->setupUi(this);
  ui->stackedWidget->setCurrentWidget(ui->pageLabel);
  ui->stackedWidget->setVisible(m_Editable);

  m_TopicDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDot | QDir::NoDotDot);

  auto model = new QFileSystemModel();
  model->setRootPath(m_TopicDir.absolutePath());

  ui->listViewNotes->setModel(model);
  ui->listViewNotes->setRootIndex(model->index(model->rootPath()));
  ui->listViewNotes->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->listViewNotes->clearSelection();

  auto selectionModel = ui->listViewNotes->selectionModel();
  connect(selectionModel, &QItemSelectionModel::currentChanged, this, &TopicWidget::on_listViewNotes_clicked);
}
//----------------------------------------------------------------------------------------------------------------------

TopicWidget::~TopicWidget()
{
  delete ui;
}
//----------------------------------------------------------------------------------------------------------------------

void TopicWidget::init()
{
  ui->listViewNotes->clearSelection();
  ui->pageLabel->setFocus();
}
//----------------------------------------------------------------------------------------------------------------------

void TopicWidget::setIndex(int index)
{
  if(nullptr == m_ToolBox) return;

  ui->lineEditName->setText(m_ToolBox->itemText(index));
  m_Index = index;
}
//----------------------------------------------------------------------------------------------------------------------

void TopicWidget::on_pushButtonEditName_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->pageEdit);
}
//----------------------------------------------------------------------------------------------------------------------

void TopicWidget::on_pushButtonSaveName_clicked()
{
  if(0 <= m_Index)
  {
    m_ToolBox->setItemText(m_Index, ui->lineEditName->text());
  }

  ui->stackedWidget->setCurrentWidget(ui->pageLabel);
}
//----------------------------------------------------------------------------------------------------------------------

void TopicWidget::on_listViewNotes_clicked(const QModelIndex &index)
{
  if(false == index.isValid()) return;

  const auto fileName = index.data().toString();

  if(false == fileName.isEmpty())
  {
    emit fileSelected(m_TopicDir.absoluteFilePath(fileName));
  }
}
//----------------------------------------------------------------------------------------------------------------------

void TopicWidget::on_toolButtonAddNote_clicked()
{
  const auto fileName = createNewFileName();
  const auto absoluteFileName = m_TopicDir.absoluteFilePath(fileName);
  auto file = QFile(absoluteFileName);

  if(true == file.exists()) return;

  auto opened = file.open(QIODevice::ReadWrite);
  file.close();

  if(true == opened)
  {

    auto selectionModel = ui->listViewNotes->selectionModel();
    auto fileSystemModel = qobject_cast<QFileSystemModel*>(ui->listViewNotes->model());
    if((nullptr != fileSystemModel) && (nullptr != selectionModel))
    {
      selectionModel->select(fileSystemModel->index(absoluteFileName), QItemSelectionModel::ClearAndSelect);
    }
    emit fileSelected(absoluteFileName);
  }
}
//----------------------------------------------------------------------------------------------------------------------

QString TopicWidget::createNewFileName()
{
  auto newFileName = m_FileTemplate;

  newFileName.replace(QString("%N"), m_TopicDir.dirName());

  {
    const auto dateTime = QDateTime::currentDateTime();
    const QString dateTimeString = dateTime.toString(m_DateTimeFormat);

    newFileName.replace(QString("%D"), dateTimeString);
  }

  newFileName.replace(QString("%C"), QString::number(m_TopicDir.count()+1));
  return newFileName;
}
//----------------------------------------------------------------------------------------------------------------------
