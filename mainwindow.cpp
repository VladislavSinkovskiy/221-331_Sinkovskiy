// mainwindow.cpp

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pindialog.h"

#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTableWidgetItem>

static QString maskStr(const QString& s)
{
    if (s.isEmpty()) return "";
    const int len = qMin(10, s.size());
    return QString::fromUtf8("•").repeated(len);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // PIN вводится один раз при старте
    PinDialog auth(this);
    auth.setWindowTitle("Аутентификация");
    if (auth.exec() != QDialog::Accepted) {
        close();
        return;
    }
    unlocked_ = true;

    setWindowTitle("Менеджер учётных данных (LR1)");

    ui->searchButton->setText("Очистить");
    ui->viewButton->setText("Просмотр");

    ui->credentialsTable->setColumnCount(3);
    ui->credentialsTable->setHorizontalHeaderLabels({"URL", "Login", "Password"});
    ui->credentialsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->credentialsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->credentialsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->credentialsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->credentialsTable->verticalHeader()->setVisible(false);

    connect(ui->searchLine, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(ui->searchButton, &QPushButton::clicked, this, &MainWindow::onClearSearchClicked);
    connect(ui->viewButton, &QPushButton::clicked, this, &MainWindow::onViewClicked);

    loadCredsFromJson("creds.json");
    populateTable();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadCredsFromJson(const QString &path)
{
    // Проверка размера файла по требованиям (>= 2 KB, т.е. >= 2048 байт)
    const QFileInfo fi(path);
    if (fi.exists()) {
        const qint64 sizeBytes = fi.size();
        if (sizeBytes < 2048) {
            QMessageBox::warning(
                this,
                "Внимание",
                "Файл учётных данных меньше 2 KB.\n"
                "Требование лабораторной: >= 2 KB (>= 2048 байт).\n\n"
                "Текущий размер: " + QString::number(sizeBytes) + " байт.\n"
                                                   "Проводник мог показать \"2 KB\" из-за округления."
                );
        }
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка",
                              "Не удалось открыть файл учётных данных: " + path +
                                  "\nПоложи creds.json рядом с exe (в папку сборки).");
        return;
    }

    const QByteArray raw = f.readAll();
    f.close();

    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(raw, &err);

    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        QMessageBox::critical(this, "Ошибка",
                              "Файл учётных данных имеет неверный формат JSON (ожидается массив объектов).");
        return;
    }

    creds_.clear();
    const QJsonArray arr = doc.array();
    creds_.reserve(arr.size());

    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();

        Cred c;
        c.url = o.value("url").toString();
        c.login = o.value("login").toString();
        c.password = o.value("password").toString();

        if (!c.url.isEmpty()) {
            creds_.push_back(std::move(c));
        }
    }

    if (creds_.size() < 10) {
        QMessageBox::warning(this, "Внимание",
                             "В creds.json меньше 10 записей. По требованиям нужно >= 10.");
    }
}

void MainWindow::populateTable()
{
    ui->credentialsTable->setRowCount(static_cast<int>(creds_.size()));

    for (int row = 0; row < creds_.size(); ++row) {
        const auto& c = creds_[row];

        auto* urlItem = new QTableWidgetItem(c.url);
        auto* loginItem = new QTableWidgetItem(maskStr(c.login));
        auto* passItem = new QTableWidgetItem(maskStr(c.password));

        urlItem->setFlags(urlItem->flags() & ~Qt::ItemIsEditable);
        loginItem->setFlags(loginItem->flags() & ~Qt::ItemIsEditable);
        passItem->setFlags(passItem->flags() & ~Qt::ItemIsEditable);

        ui->credentialsTable->setItem(row, 0, urlItem);
        ui->credentialsTable->setItem(row, 1, loginItem);
        ui->credentialsTable->setItem(row, 2, passItem);
    }
}

void MainWindow::applyFilter(const QString &text)
{
    const QString t = text.trimmed();

    for (int row = 0; row < creds_.size(); ++row) {
        const bool match = t.isEmpty() || creds_[row].url.contains(t, Qt::CaseInsensitive);
        ui->credentialsTable->setRowHidden(row, !match);
    }
}

int MainWindow::selectedRow() const
{
    const auto items = ui->credentialsTable->selectedItems();
    if (items.isEmpty()) return -1;
    return items.first()->row();
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    applyFilter(text);
}

void MainWindow::onClearSearchClicked()
{
    ui->searchLine->clear();
}

void MainWindow::onViewClicked()
{
    if (!unlocked_) {
        QMessageBox::warning(this, "Доступ запрещён", "Хранилище не разблокировано.");
        return;
    }

    const int row = selectedRow();
    if (row < 0 || row >= creds_.size()) {
        QMessageBox::information(this, "Просмотр", "Выбери запись в таблице.");
        return;
    }

    const auto& c = creds_[row];
    QMessageBox::information(this, "Учётные данные",
                             "URL: " + c.url +
                                 "\nLogin: " + c.login +
                                 "\nPassword: " + c.password);
}
