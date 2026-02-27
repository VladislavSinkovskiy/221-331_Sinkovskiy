#include "pindialog.h"
#include "ui_pindialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

static const QString kMasterPin = "777";

PinDialog::PinDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PinDialog)
{
    ui->setupUi(this);

    setModal(true);
    setWindowTitle("Подтвердите PIN");

    ui->pinLineEdit->setEchoMode(QLineEdit::Password);
    ui->pinLineEdit->setMaxLength(8);
    ui->statusLabel->clear();

    disconnect(ui->unlockButton, nullptr, this, nullptr);

    connect(ui->unlockButton, &QPushButton::clicked,
            this, &PinDialog::onUnlockClicked);

    connect(ui->pinLineEdit, &QLineEdit::returnPressed,
            this, &PinDialog::onUnlockClicked);
}

PinDialog::~PinDialog()
{
    delete ui;
}

QString PinDialog::pin() const
{
    return ui->pinLineEdit->text();
}

void PinDialog::onUnlockClicked()
{
    const QString entered = ui->pinLineEdit->text().trimmed();

    if (entered.isEmpty()) {
        ui->statusLabel->setText("PIN не должен быть пустым.");
        ui->statusLabel->setStyleSheet("color: red;");
        return;
    }

    if (entered != kMasterPin) {
        ui->statusLabel->setText("Неверный PIN-код.");
        ui->statusLabel->setStyleSheet("color: red;");
        return;
    }

    accept();
}
