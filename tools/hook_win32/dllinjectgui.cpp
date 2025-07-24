#include "dllinject.h"
#include "dllinjectgui.h"
#include "./ui_dllinjectgui.h"
#include <string>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QListWidgetItem>
#include <QKeyEvent>
#include <QDebug>
#include <QTimer>
#include <windows.h>
#include <tlhelp32.h>

const char *DLL_PATH = "hook_backend\\hook.dll";

// Helper: 获取所有进程名和PID
QList<QPair<QString, DWORD>> getAllProcesses() {
    QList<QPair<QString, DWORD>> result;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return result;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(hSnap, &pe)) {
        do {
            result.append(qMakePair(QString::fromWCharArray(pe.szExeFile), pe.th32ProcessID));
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return result;
}

DLLInjectGUI::DLLInjectGUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DLLInjectGUI) {
    ui->setupUi((QMainWindow *)this);
    //setFixedSize(width(), height()); // 固定窗口大小

    // 初始化进程列表
    refreshProcessList();

    // 搜索框回车
    connect(ui->txtSearch, &QLineEdit::returnPressed, this, &DLLInjectGUI::onSearch);
    // 搜索、刷新按钮
    connect(ui->btnSearch, &QPushButton::clicked, this, &DLLInjectGUI::onSearch);
    connect(ui->btnRefresh, &QPushButton::clicked, this, [this](){refreshProcessList();});

    // 列表选中变化
    connect(ui->lstProcess, &QListWidget::itemSelectionChanged, this, &DLLInjectGUI::onSelectionChanged);

    // 注入按钮
    connect(ui->btnInject, &QPushButton::clicked, this, &DLLInjectGUI::onInject);

    ui->btnInject->setEnabled(false);
    for(int tm=180;tm<1500;tm+=100){ // TODO: 解决缩放后窗口边缘不显示
        QTimer::singleShot(tm, this, DLLInjectGUI::updateLayoutSize);
    }
}

DLLInjectGUI::~DLLInjectGUI() {
    delete ui;
}

void DLLInjectGUI::refreshProcessList(const QString &filter) {
    ui->lstProcess->clear();
    processList = getAllProcesses();
    for (const auto &pair : processList) {
        QString itemText = QString("%1 (%2)").arg(pair.first).arg(pair.second);
        if (filter.isEmpty() || itemText.contains(filter, Qt::CaseInsensitive)) {
            QListWidgetItem *item = new QListWidgetItem(itemText, ui->lstProcess);
            item->setData(Qt::UserRole, QVariant::fromValue(static_cast<qulonglong>(pair.second)));
        }
    }
}

void DLLInjectGUI::onSearch() {
    QString filter = ui->txtSearch->text();
    refreshProcessList(filter);
}

void DLLInjectGUI::onSelectionChanged() {
    ui->btnInject->setEnabled(!ui->lstProcess->selectedItems().isEmpty());
}

void DLLInjectGUI::onInject() {
    QList<QListWidgetItem*> selected = ui->lstProcess->selectedItems();
    int success = 0, fail = 0;
    std::string msgs;
    for (auto *item : selected) {
        DWORD pid = item->data(Qt::UserRole).toUInt();
        if (injectDLL(pid, DLL_PATH)){
            success++;
        } else {
            fail++;
            if(lastErrMsg){
                msgs+=lastErrMsg;
                msgs+='\n';
            }
        }
    }
    if(!msgs.empty()) msgs = "Errors:\n" + msgs;
    QMessageBox::information(this, "DLL Injection",
                             QString("Injection finished.\nSuccess: %1\nFailed: %2\n%3")\
                                 .arg(success).arg(fail).arg(msgs.c_str()));
}

void DLLInjectGUI::updateLayoutSize(){
    QLayout *layout = ui->verticalLayout;
    if (layout) {
        QRect curGeometry = layout->geometry();

        // 新布局区域宽高为原宽高加差值，高度同理
        QRect newGeometry(
            curGeometry.x(),
            curGeometry.y(),
            this->size().width() - 6,
            this->size().height() - 6
        );

        layout->setGeometry(newGeometry);
    }
}
void DLLInjectGUI::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);
    updateLayoutSize();
}
void DLLInjectGUI::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Control) {
        ui->lstProcess->setSelectionMode(QAbstractItemView::MultiSelection);
    } else if (event->key() == Qt::Key_Shift) {
        ui->lstProcess->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    // 传递给基类处理其它按键事件
    QMainWindow::keyPressEvent(event);
}

void DLLInjectGUI::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift) {
        ui->lstProcess->setSelectionMode(QAbstractItemView::SingleSelection);
    }
    QMainWindow::keyReleaseEvent(event);
}