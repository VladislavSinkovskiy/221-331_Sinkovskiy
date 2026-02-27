#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    struct Cred {
        QString url;
        QString login;
        QString password;
    };

    Ui::MainWindow *ui;
    QVector<Cred> creds_;

    bool unlocked_ = false;
    void loadCredsFromJson(const QString& path);
    void populateTable();
    void applyFilter(const QString& text);
    int selectedRow() const;

private slots:
    void onSearchTextChanged(const QString& text);
    void onClearSearchClicked();
    void onViewClicked();
};

#endif
