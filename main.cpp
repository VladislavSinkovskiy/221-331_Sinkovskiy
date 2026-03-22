#include "mainwindow.h"
#include "security_state.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <winnt.h>
#endif

#include <QApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QString>

#ifdef _WIN32
static bool verifyTextSectionSha256(QString *errorOut = nullptr)
{
    wchar_t exePath[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH)) {
        if (errorOut) {
            *errorOut = "Не удалось получить путь к исполняемому файлу";
        }
        return false;
    }

    QFile file(QString::fromWCharArray(exePath));
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = "Не удалось открыть исполняемый файл";
        }
        return false;
    }

    const QByteArray fileData = file.readAll();
    file.close();

    if (fileData.size() < static_cast<int>(sizeof(IMAGE_DOS_HEADER))) {
        if (errorOut) {
            *errorOut = "Файл слишком мал, DOS-заголовок отсутствует";
        }
        return false;
    }

    const auto *base = reinterpret_cast<const unsigned char*>(fileData.constData());

    const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        if (errorOut) {
            *errorOut = "Некорректная DOS-сигнатура";
        }
        return false;
    }

    if (dos->e_lfanew <= 0 ||
        dos->e_lfanew + static_cast<int>(sizeof(IMAGE_NT_HEADERS)) > fileData.size()) {
        if (errorOut) {
            *errorOut = "Некорректный сдвиг NT-заголовка";
        }
        return false;
    }

    const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        if (errorOut) {
            *errorOut = "Некорректная NT-сигнатура";
        }
        return false;
    }

    const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    const WORD secCount = nt->FileHeader.NumberOfSections;
    const IMAGE_SECTION_HEADER *textSec = nullptr;

    for (WORD i = 0; i < secCount; ++i) {
        char name[9]{};
        memcpy(name, sec[i].Name, 8);

        if (strcmp(name, ".text") == 0) {
            textSec = &sec[i];
            break;
        }
    }

    if (!textSec) {
        if (errorOut) {
            *errorOut = "Секция .text не найдена ";
        }
        return false;
    }

    const DWORD virtualAddress = textSec->VirtualAddress;
    const DWORD virtualSize = textSec->Misc.VirtualSize;
    const DWORD rawOffset = textSec->PointerToRawData;
    const DWORD rawSize = textSec->SizeOfRawData;

    if (rawOffset + rawSize > static_cast<DWORD>(fileData.size())) {
        if (errorOut) {
            *errorOut = "Некорректные данные секции .text в файле";
        }
        return false;
    }

    qDebug() << "Виртуальный адрес .text: 0x" + QByteArray::number(virtualAddress, 16).toUpper();
    qDebug() << "Размер .text (VirtualSize):" << virtualSize << "байт";

    const QByteArray textData(fileData.constData() + rawOffset, rawSize);
    const QByteArray calculatedHash =
        QCryptographicHash::hash(textData, QCryptographicHash::Sha256);

    qDebug() << "Calculated SHA256:" << calculatedHash.toHex();

    const QByteArray referenceHash = QByteArray::fromHex(
        "85053fa9d717a7c766989455d25136ef8eeb9669a77c7df10ee2c1578038e602"
        );
    if (calculatedHash != referenceHash) {
        if (errorOut) {
            *errorOut = QString("Контрольная сумма не совпала!\n"
                                "Ожидалось: %1\n"
                                "Получено:  %2")
                            .arg(QString(referenceHash.toHex()))
                            .arg(QString(calculatedHash.toHex()));
        }
        return false;
    }

    return true;
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (SecurityState::isAttackDetected()) {
        QMessageBox::critical(
            nullptr,
            "Предупреждение безопасности",
            SecurityState::attackMessage()
            );
        return 0;
    }

#ifdef _WIN32
    //if (IsDebuggerPresent()) {
        //QMessageBox::critical(
            //nullptr,
            //"Предупреждение!",
            //"Обнаружен отладчик! Приложение будет закрыто."
            //);
       // return 0;
   // }

    QString err;
    if (!verifyTextSectionSha256(&err)) {
        QMessageBox::critical(
            nullptr,
            "Ошибка безопасности",
            "Обнаружена модификация приложения:\n" + err
            );
        return 0;
    }
#endif

    MainWindow w;
    if (!w.isInitialized()) {
        return 0;
    }

    w.show();
    return a.exec();
}
