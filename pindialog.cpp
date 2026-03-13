#include "pindialog.h"
#include "ui_pindialog.h"
#include "security_state.h"

#include <QLineEdit>
#include <QPushButton>

PinDialog::PinDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PinDialog)
{
    ui->setupUi(this);

    setModal(true);
    setWindowTitle("Подтвердите PIN");

    ui->pinLineEdit->setEchoMode(QLineEdit::Password);
    ui->pinLineEdit->setMaxLength(16);

    if (SecurityState::isAttackDetected()) {
        ui->statusLabel->setText(SecurityState::attackMessage());
        ui->statusLabel->setStyleSheet("color: red;");
        ui->pinLineEdit->setEnabled(false);
        ui->unlockButton->setEnabled(false);
        return;
    }

    ui->statusLabel->clear();

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
    return ui->pinLineEdit->text().trimmed();
}

void PinDialog::onUnlockClicked()
{
    const QString entered = ui->pinLineEdit->text().trimmed();

    if (entered.isEmpty()) {
        ui->statusLabel->setText("PIN не должен быть пустым.");
        ui->statusLabel->setStyleSheet("color: red;");
        return;
    }

    accept();
}
