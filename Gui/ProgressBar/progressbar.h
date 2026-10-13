#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QWidget>

namespace Ui {
class ProgressBar;
}

class ProgressBar : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressBar(QWidget *parent = nullptr);
    ~ProgressBar();

    void setValue(int progress);
    void setText(const QString &text);
    //忙指示（扫描等无法估算进度的阶段显示忙碌动画）
    void setBusy(bool busy);
    //复位（进度归零，文本显示"就绪"）
    void reset();

protected:
    void initGui();

private:
    Ui::ProgressBar *ui;
};

#endif // PROGRESSBAR_H
