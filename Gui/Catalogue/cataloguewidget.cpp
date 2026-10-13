#include "cataloguewidget.h"
#include "ui_cataloguewidget.h"
#include "catalogueuidata.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

CatalogueWidget::CatalogueWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CatalogueWidget)
{
    ui->setupUi(this);
    initGui();
    initConnect();
}

CatalogueWidget::~CatalogueWidget()
{
    delete ui;
}

void CatalogueWidget::btn_SkimthroughA_Clicked()
{
    m_dir_A = openCatalogue(m_dir_A);
    ui->le_A->setText(m_dir_A);
    emit setDirA(m_dir_A);
}

void CatalogueWidget::btn_SkimthrougnB_Clicked()
{
    m_dir_B = openCatalogue(m_dir_B);
    ui->le_B->setText(m_dir_B);
    emit setDirB(m_dir_B);
}

void CatalogueWidget::browseDirA()
{
    btn_SkimthroughA_Clicked();
}

void CatalogueWidget::browseDirB()
{
    btn_SkimthrougnB_Clicked();
}

void CatalogueWidget::setDirectoryA(const QString& dir)
{
    m_dir_A = dir;
    ui->le_A->setText(dir);
    //不主动发信号，由调用方（loadSettings）统一同步引擎
}

void CatalogueWidget::setDirectoryB(const QString& dir)
{
    m_dir_B = dir;
    ui->le_B->setText(dir);
    //不主动发信号，由调用方（loadSettings）统一同步引擎
}

void CatalogueWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void CatalogueWidget::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;
    const QString path = urls.first().toLocalFile();
    if (path.isEmpty() || !QFileInfo(path).isDir())
        return;

    //根据落点判断设置目录 A 还是 B
    if (ui->le_A->rect().contains(ui->le_A->mapFrom(this, event->pos())))
    {
        m_dir_A = path;
        ui->le_A->setText(path);
        emit setDirA(path);
    }
    else if (ui->le_B->rect().contains(ui->le_B->mapFrom(this, event->pos())))
    {
        m_dir_B = path;
        ui->le_B->setText(path);
        emit setDirB(path);
    }
}

void CatalogueWidget::initGui()
{
    //设置控件最小高
    ui->le_A->setFixedHeight(le_A_FixHeighet);
    ui->le_B->setFixedHeight(le_B_FixHeighet);
    ui->lb_A->setFixedHeight(lb_A_FixHeight);
    ui->lb_B->setFixedHeight(lb_B_FixHeight);
    ui->btn_A->setFixedHeight(btn_A_FixHeight);
    ui->btn_B->setFixedHeight(btn_B_FixHeight);

    //设置输入框默认显示
    ui->le_A->setPlaceholderText(le_A_default_text);
    ui->le_B->setPlaceholderText(le_B_default_text);

    //支持拖拽目录到输入框
    setAcceptDrops(true);
}

void CatalogueWidget::initConnect()
{
    connect(ui->btn_A,&QPushButton::clicked,this,&CatalogueWidget::btn_SkimthroughA_Clicked);
    connect(ui->btn_B,&QPushButton::clicked,this,&CatalogueWidget::btn_SkimthrougnB_Clicked);
}

QString CatalogueWidget::openCatalogue(const QString& startDir)
{
    const QString start = startDir.isEmpty() ? QDir::homePath() : startDir;
    QString dir = QFileDialog::getExistingDirectory(this, QString("选择目录"), start,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    return dir;
}
