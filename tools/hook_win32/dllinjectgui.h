#ifndef DLLINJECTGUI_H
#define DLLINJECTGUI_H

#include <QMainWindow>
#include <QList>
#include <QPair>
#include <windows.h>

extern const char *DLL_PATH;

QT_BEGIN_NAMESPACE
namespace Ui {
class DLLInjectGUI;
}
QT_END_NAMESPACE

class DLLInjectGUI : public QMainWindow {
    Q_OBJECT

public:
    explicit DLLInjectGUI(QWidget *parent = nullptr);
    ~DLLInjectGUI();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Ui::DLLInjectGUI *ui;

    // 进程列表缓存，存储进程名和PID
    QList<QPair<QString, DWORD>> processList;

    // 刷新进程列表，支持过滤
    void refreshProcessList(const QString &filter = QString());
    void updateLayoutSize();

private slots:
    void onSearch();
    void onSelectionChanged();
    void onInject();
};

#endif // DLLINJECTGUI_H
