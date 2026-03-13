#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <QByteArray>
#include <QString>

namespace CryptoUtils {

QByteArray deriveKeyFromPin(const QString& pin);

bool encryptAes256Cbc(const QByteArray& plainText,
                      const QByteArray& key32,
                      QByteArray* outBlob);

bool decryptAes256Cbc(const QByteArray& blob,
                      const QByteArray& key32,
                      QByteArray* outPlainText);
void secureZero(QByteArray& data);
void secureZero(QString& data);

}

#endif
