#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
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

    bool isInitialized() const;

private:
    struct Cred {
        QString url;
        QByteArray encLogin;
        QByteArray encPassword;
    };

    Ui::MainWindow *ui;
    QVector<Cred> creds_;

    bool unlocked_ = false;
    bool initOk_ = false;

    bool authenticateAndLoad();
    bool loadCredsFromEncryptedFile(const QString& path, const QString& pin);
    void populateTable();
    void applyFilter(const QString& text);
    int selectedRow() const;

    bool requestPinAndDecryptField(int row, bool decryptLogin, QString* outValue);

private slots:
    void onSearchTextChanged(const QString& text);
    void onClearSearchClicked();
    void onViewClicked();
};

#endif
