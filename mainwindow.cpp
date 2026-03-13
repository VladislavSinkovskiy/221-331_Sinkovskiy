#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pindialog.h"
#include "crypto_utils.h"
#include "security_state.h"

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QVBoxLayout>

static QString maskStr(const QString& s)
{
    if (s.isEmpty()) return "";
    const int len = qMin(10, s.size());
    return QString::fromUtf8("•").repeated(len);
}

static void triggerSecurityBlock(QWidget* parent, const QString& message)
{
    SecurityState::setAttackDetected(message);
    QMessageBox::critical(parent, "Предупреждение безопасности", message);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    if (!authenticateAndLoad()) {
        return;
    }

    populateTable();
    initOk_ = true;
}

MainWindow::~MainWindow()
{
    for (Cred& c : creds_) {
        CryptoUtils::secureZero(c.encLogin);
        CryptoUtils::secureZero(c.encPassword);
    }
    creds_.clear();
    delete ui;
}

bool MainWindow::isInitialized() const
{
    return initOk_;
}

bool MainWindow::authenticateAndLoad()
{
    if (SecurityState::isAttackDetected()) {
        QMessageBox::critical(
            this,
            "Предупреждение безопасности",
            SecurityState::attackMessage()
            );
        return false;
    }

    const QFileInfo fi("creds.enc");
    if (!fi.exists()) {
        triggerSecurityBlock(
            this,
            "Файл creds.enc не найден. Ввод PIN-кода заблокирован."
            );
        return false;
    }

    PinDialog auth(this);
    auth.setWindowTitle("Аутентификация");

    if (auth.exec() != QDialog::Accepted) {
        return false;
    }

    QString pin = auth.pin();

    const bool ok = loadCredsFromEncryptedFile("creds.enc", pin);
    CryptoUtils::secureZero(pin);

    if (!ok) {
        triggerSecurityBlock(
            this,
            "Обнаружена ошибка проверки PIN-кода или повреждение защищённого файла. Доступ заблокирован."
            );
        return false;
    }

    unlocked_ = true;
    return true;
}

bool MainWindow::loadCredsFromEncryptedFile(const QString& path, const QString& pin)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray encryptedFile = f.readAll();
    f.close();

    if (encryptedFile.size() <= 16) {
        CryptoUtils::secureZero(encryptedFile);
        return false;
    }

    QByteArray fileKey = CryptoUtils::deriveKeyFromPin(pin);

    QByteArray plainJson;
    if (!CryptoUtils::decryptAes256Cbc(encryptedFile, fileKey, &plainJson)) {
        CryptoUtils::secureZero(fileKey);
        CryptoUtils::secureZero(encryptedFile);
        return false;
    }

    QByteArray memoryKey = CryptoUtils::deriveKeyFromPin(pin + "::memory-layer::v1");

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(plainJson, &err);

    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        CryptoUtils::secureZero(fileKey);
        CryptoUtils::secureZero(memoryKey);
        CryptoUtils::secureZero(encryptedFile);
        CryptoUtils::secureZero(plainJson);
        return false;
    }

    creds_.clear();

    const QJsonArray arr = doc.array();
    creds_.reserve(arr.size());

    for (const auto& v : arr) {
        if (!v.isObject()) {
            continue;
        }

        const QJsonObject o = v.toObject();

        QString url = o.value("url").toString();
        QString login = o.value("login").toString();
        QString password = o.value("password").toString();

        if (url.isEmpty()) {
            CryptoUtils::secureZero(login);
            CryptoUtils::secureZero(password);
            continue;
        }

        QByteArray loginUtf8 = login.toUtf8();
        QByteArray passwordUtf8 = password.toUtf8();

        QByteArray encLogin;
        QByteArray encPassword;

        const bool okLogin = CryptoUtils::encryptAes256Cbc(loginUtf8, memoryKey, &encLogin);
        const bool okPassword = CryptoUtils::encryptAes256Cbc(passwordUtf8, memoryKey, &encPassword);

        CryptoUtils::secureZero(loginUtf8);
        CryptoUtils::secureZero(passwordUtf8);
        CryptoUtils::secureZero(login);
        CryptoUtils::secureZero(password);

        if (!okLogin || !okPassword) {
            CryptoUtils::secureZero(encLogin);
            CryptoUtils::secureZero(encPassword);
            CryptoUtils::secureZero(fileKey);
            CryptoUtils::secureZero(memoryKey);
            CryptoUtils::secureZero(encryptedFile);
            CryptoUtils::secureZero(plainJson);
            creds_.clear();
            return false;
        }

        Cred c;
        c.url = std::move(url);
        c.encLogin = std::move(encLogin);
        c.encPassword = std::move(encPassword);

        creds_.push_back(std::move(c));
    }

    CryptoUtils::secureZero(fileKey);
    CryptoUtils::secureZero(memoryKey);
    CryptoUtils::secureZero(encryptedFile);
    CryptoUtils::secureZero(plainJson);

    return !creds_.isEmpty();
}

void MainWindow::populateTable()
{
    ui->credentialsTable->setRowCount(static_cast<int>(creds_.size()));

    for (int row = 0; row < creds_.size(); ++row) {
        const auto& c = creds_[row];

        auto* urlItem = new QTableWidgetItem(c.url);
        auto* loginItem = new QTableWidgetItem(maskStr("login"));
        auto* passItem = new QTableWidgetItem(maskStr("password"));

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

bool MainWindow::requestPinAndDecryptField(int row, bool decryptLogin, QString* outValue)
{
    if (SecurityState::isAttackDetected()) {
        QMessageBox::critical(
            this,
            "Предупреждение безопасности",
            SecurityState::attackMessage()
            );
        return false;
    }

    if (!outValue) {
        return false;
    }

    if (row < 0 || row >= creds_.size()) {
        return false;
    }

    PinDialog auth(this);
    auth.setWindowTitle("Подтвердите PIN");

    if (auth.exec() != QDialog::Accepted) {
        return false;
    }

    QString pin = auth.pin();
    QByteArray memoryKey = CryptoUtils::deriveKeyFromPin(pin + "::memory-layer::v1");
    CryptoUtils::secureZero(pin);

    QByteArray plainSecret;
    const QByteArray& enc = decryptLogin ? creds_[row].encLogin : creds_[row].encPassword;

    const bool ok = CryptoUtils::decryptAes256Cbc(enc, memoryKey, &plainSecret);
    CryptoUtils::secureZero(memoryKey);

    if (!ok) {
        triggerSecurityBlock(
            this,
            "Обнаружена ошибка повторной аутентификации при доступе к защищённым данным. Доступ заблокирован."
            );
        return false;
    }

    *outValue = QString::fromUtf8(plainSecret);
    CryptoUtils::secureZero(plainSecret);

    return true;
}

void MainWindow::onViewClicked()
{
    if (SecurityState::isAttackDetected()) {
        QMessageBox::critical(
            this,
            "Предупреждение безопасности",
            SecurityState::attackMessage()
            );
        return;
    }

    if (!unlocked_) {
        QMessageBox::warning(this, "Доступ запрещён", "Хранилище не разблокировано.");
        return;
    }

    const int row = selectedRow();
    if (row < 0 || row >= creds_.size()) {
        QMessageBox::information(this, "Просмотр", "Выбери запись в таблице.");
        return;
    }

    PinDialog auth(this);
    auth.setWindowTitle("Подтвердите PIN");

    if (auth.exec() != QDialog::Accepted) {
        return;
    }

    QString pin = auth.pin();
    QByteArray memoryKey = CryptoUtils::deriveKeyFromPin(pin + "::memory-layer::v1");
    CryptoUtils::secureZero(pin);

    QByteArray plainLogin;
    QByteArray plainPassword;

    const bool okLogin = CryptoUtils::decryptAes256Cbc(creds_[row].encLogin, memoryKey, &plainLogin);
    const bool okPassword = CryptoUtils::decryptAes256Cbc(creds_[row].encPassword, memoryKey, &plainPassword);
    CryptoUtils::secureZero(memoryKey);

    if (!okLogin || !okPassword) {
        CryptoUtils::secureZero(plainLogin);
        CryptoUtils::secureZero(plainPassword);

        triggerSecurityBlock(
            this,
            "Обнаружена ошибка повторной аутентификации при доступе к защищённым данным. Доступ заблокирован."
            );
        return;
    }

    QString login = QString::fromUtf8(plainLogin);
    QString password = QString::fromUtf8(plainPassword);

    CryptoUtils::secureZero(plainLogin);
    CryptoUtils::secureZero(plainPassword);

    const auto& c = creds_[row];

    QDialog dialog(this);
    dialog.setWindowTitle("Учётные данные");
    dialog.setModal(true);
    dialog.setMinimumWidth(560);

    auto *mainLayout = new QVBoxLayout(&dialog);
    auto *grid = new QGridLayout();

    auto *urlTitleLabel = new QLabel("URL:", &dialog);
    auto *urlValueLabel = new QLabel(c.url, &dialog);
    urlValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *loginTitleLabel = new QLabel("Login:", &dialog);
    auto *loginValueLabel = new QLabel(login, &dialog);
    loginValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *copyLoginButton = new QPushButton("Копировать", &dialog);

    auto *passwordTitleLabel = new QLabel("Password:", &dialog);
    auto *passwordValueLabel = new QLabel(password, &dialog);
    passwordValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *copyPasswordButton = new QPushButton("Копировать", &dialog);

    grid->addWidget(urlTitleLabel, 0, 0);
    grid->addWidget(urlValueLabel, 0, 1);

    grid->addWidget(loginTitleLabel, 1, 0);
    grid->addWidget(loginValueLabel, 1, 1);
    grid->addWidget(copyLoginButton, 1, 2);

    grid->addWidget(passwordTitleLabel, 2, 0);
    grid->addWidget(passwordValueLabel, 2, 1);
    grid->addWidget(copyPasswordButton, 2, 2);

    grid->setColumnStretch(1, 1);

    mainLayout->addLayout(grid);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();

    auto *okButton = new QPushButton("OK", &dialog);
    buttonsLayout->addWidget(okButton);

    mainLayout->addLayout(buttonsLayout);

    connect(copyLoginButton, &QPushButton::clicked, &dialog, [this, login]() {
        QApplication::clipboard()->setText(login);
        if (statusBar()) {
            statusBar()->showMessage("Логин скопирован в буфер обмена", 3000);
        }
    });

    connect(copyPasswordButton, &QPushButton::clicked, &dialog, [this, password]() {
        QApplication::clipboard()->setText(password);
        if (statusBar()) {
            statusBar()->showMessage("Пароль скопирован в буфер обмена", 3000);
        }
    });

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();

    CryptoUtils::secureZero(login);
    CryptoUtils::secureZero(password);
}
