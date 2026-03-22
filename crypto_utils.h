#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <QByteArray>
#include <QString>
#include <QIODevice>

namespace CryptoUtils {

// Ключ = SHA-256(PIN)
QByteArray deriveKeyFromPin(const QString& pin);

// AES-256-CBC.
// Формат blob: [16 байт IV][ciphertext]
bool encryptAes256Cbc(const QByteArray& plainText,
                      const QByteArray& key32,
                      QByteArray* outBlob);

bool decryptAes256Cbc(const QByteArray& blob,
                      const QByteArray& key32,
                      QByteArray* outPlainText);

// Потоковая расшифровка из файла/устройства по блокам.
// Формат файла: [16 байт IV][ciphertext]
bool decryptAes256CbcFromDevice(QIODevice& device,
                                const QByteArray& key32,
                                QByteArray* outPlainText);

void secureZero(QByteArray& data);
void secureZero(QString& data);

}

#endif
