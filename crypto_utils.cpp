#include "crypto_utils.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <QFileDevice>

namespace CryptoUtils {

QByteArray deriveKeyFromPin(const QString& pin)
{
    QByteArray pinBytes = pin.toUtf8();

    unsigned char key[32];
    SHA256(reinterpret_cast<const unsigned char*>(pinBytes.constData()),
           static_cast<size_t>(pinBytes.size()),
           key);

    QByteArray result(reinterpret_cast<const char*>(key), 32);

    OPENSSL_cleanse(key, sizeof(key));
    secureZero(pinBytes);

    return result;
}

bool encryptAes256Cbc(const QByteArray& plainText,
                      const QByteArray& key32,
                      QByteArray* outBlob)
{
    if (!outBlob || key32.size() != 32) {
        return false;
    }

    unsigned char iv[16];
    if (RAND_bytes(iv, sizeof(iv)) != 1) {
        OPENSSL_cleanse(iv, sizeof(iv));
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        OPENSSL_cleanse(iv, sizeof(iv));
        return false;
    }

    QByteArray cipherText;
    cipherText.resize(plainText.size() + EVP_MAX_BLOCK_LENGTH);

    int len = 0;
    int totalLen = 0;

    bool ok = false;

    do {
        if (EVP_EncryptInit_ex(ctx,
                               EVP_aes_256_cbc(),
                               nullptr,
                               reinterpret_cast<const unsigned char*>(key32.constData()),
                               iv) != 1) {
            break;
        }

        if (EVP_EncryptUpdate(ctx,
                              reinterpret_cast<unsigned char*>(cipherText.data()),
                              &len,
                              reinterpret_cast<const unsigned char*>(plainText.constData()),
                              plainText.size()) != 1) {
            break;
        }
        totalLen = len;

        if (EVP_EncryptFinal_ex(ctx,
                                reinterpret_cast<unsigned char*>(cipherText.data()) + totalLen,
                                &len) != 1) {
            break;
        }
        totalLen += len;

        cipherText.resize(totalLen);

        outBlob->clear();
        outBlob->append(reinterpret_cast<const char*>(iv), 16);
        outBlob->append(cipherText);

        ok = true;
    } while (false);

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(iv, sizeof(iv));
    secureZero(cipherText);

    return ok;
}

bool decryptAes256Cbc(const QByteArray& blob,
                      const QByteArray& key32,
                      QByteArray* outPlainText)
{
    if (!outPlainText || key32.size() != 32 || blob.size() <= 16) {
        return false;
    }

    const unsigned char* iv =
        reinterpret_cast<const unsigned char*>(blob.constData());
    const unsigned char* cipher =
        reinterpret_cast<const unsigned char*>(blob.constData() + 16);
    const int cipherLen = blob.size() - 16;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    QByteArray plainText;
    plainText.resize(cipherLen + EVP_MAX_BLOCK_LENGTH);

    int len = 0;
    int totalLen = 0;

    bool ok = false;

    do {
        if (EVP_DecryptInit_ex(ctx,
                               EVP_aes_256_cbc(),
                               nullptr,
                               reinterpret_cast<const unsigned char*>(key32.constData()),
                               iv) != 1) {
            break;
        }

        if (EVP_DecryptUpdate(ctx,
                              reinterpret_cast<unsigned char*>(plainText.data()),
                              &len,
                              cipher,
                              cipherLen) != 1) {
            break;
        }
        totalLen = len;

        if (EVP_DecryptFinal_ex(ctx,
                                reinterpret_cast<unsigned char*>(plainText.data()) + totalLen,
                                &len) != 1) {
            break;
        }
        totalLen += len;

        plainText.resize(totalLen);
        *outPlainText = plainText;
        ok = true;
    } while (false);

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        secureZero(plainText);
    }

    return ok;
}

bool decryptAes256CbcFromDevice(QIODevice& device,
                                const QByteArray& key32,
                                QByteArray* outPlainText)
{
    if (!outPlainText || key32.size() != 32 || !device.isOpen()) {
        return false;
    }

    QByteArray ivBytes = device.read(16);
    if (ivBytes.size() != 16) {
        secureZero(ivBytes);
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        secureZero(ivBytes);
        return false;
    }

    constexpr qint64 kChunkSize = 4096;

    QByteArray plainText;
    QByteArray inChunk;
    QByteArray outChunk;
    outChunk.resize(static_cast<int>(kChunkSize) + EVP_MAX_BLOCK_LENGTH);

    bool streamOk = true;
    bool ok = false;

    do {
        if (EVP_DecryptInit_ex(ctx,
                               EVP_aes_256_cbc(),
                               nullptr,
                               reinterpret_cast<const unsigned char*>(key32.constData()),
                               reinterpret_cast<const unsigned char*>(ivBytes.constData())) != 1) {
            break;
        }

        while (!device.atEnd()) {
            inChunk = device.read(kChunkSize);

            if (inChunk.isEmpty()) {
                break;
            }

            int outLen = 0;
            if (EVP_DecryptUpdate(ctx,
                                  reinterpret_cast<unsigned char*>(outChunk.data()),
                                  &outLen,
                                  reinterpret_cast<const unsigned char*>(inChunk.constData()),
                                  inChunk.size()) != 1) {
                streamOk = false;
                secureZero(inChunk);
                break;
            }

            if (outLen > 0) {
                plainText.append(outChunk.constData(), outLen);
            }

            secureZero(inChunk);
        }

        if (!streamOk) {
            break;
        }

        int finalLen = 0;
        if (EVP_DecryptFinal_ex(ctx,
                                reinterpret_cast<unsigned char*>(outChunk.data()),
                                &finalLen) != 1) {
            break;
        }

        if (finalLen > 0) {
            plainText.append(outChunk.constData(), finalLen);
        }

        *outPlainText = plainText;
        ok = true;
    } while (false);

    EVP_CIPHER_CTX_free(ctx);

    secureZero(ivBytes);
    secureZero(inChunk);
    secureZero(outChunk);

    if (!ok) {
        secureZero(plainText);
    }

    return ok;
}

void secureZero(QByteArray& data)
{
    if (!data.isEmpty()) {
        OPENSSL_cleanse(data.data(), static_cast<size_t>(data.size()));
        data.clear();
        data.squeeze();
    }
}

void secureZero(QString& data)
{
    if (!data.isEmpty()) {
        for (QChar& ch : data) {
            ch = QChar(u'\0');
        }
        data.clear();
        data.squeeze();
    }
}

}
