#include "progressbar.h"
#include "ui_progressbar.h"
#include "progressuidata.h"

ProgressBar::ProgressBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProgressBar)
{
    ui->setupUi(this);
    initGui();
}

ProgressBar::~ProgressBar()
{
    delete ui;
}

void ProgressBar::setValue(int progress)
{
    ui->progress->setValue(progress);
}

void ProgressBar::setText(const QString &text)
{
    ui->lb_text->setText(text);
}

void ProgressBar::setBusy(bool busy)
{
    //busy 时 range(0,0) 使 QProgressBar 进入不确定进度动画
    ui->progress->setRange(0, busy ? 0 : PROGRESS_MAX);
    if (!busy)
        ui->progress->setValue(PROGRESS_MIN);
}

void ProgressBar::reset()
{
    setBusy(false);
    ui->progress->setValue(PROGRESS_MIN);
    ui->lb_text->setText(PROGRESS_COMPETE_TEXT);
}

void ProgressBar::initGui()
{
    ui->progress->setRange(PROGRESS_MIN,PROGRESS_MAX);
    ui->progress->setValue(PROGRESS_MIN);
    ui->lb_text->setText(PROGRESS_COMPETE_TEXT);
}
