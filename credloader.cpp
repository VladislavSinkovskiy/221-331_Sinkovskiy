#include "credloader.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


bool loadCredsFromJson(const QString& path, QVector<creditem>& outCreds, QString* err)
{
    outCreds.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = "Не удалось открыть файл: " + f.errorString();
        return false;
    }

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
    if (doc.isNull() || !doc.isObject()) {
        if (err) *err = "Ошибка парсинга JSON: " + pe.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("creds") || !root.value("creds").isArray()) {
        if (err) *err = "Неверный формат: ожидается объект с массивом поля 'creds'.";
        return false;
    }

    const QJsonArray arr = root.value("creds").toArray();
    outCreds.reserve(arr.size());

    for (const auto& v : arr) {
        if (!v.isObject()) continue;

        const QJsonObject o = v.toObject();
        const QString url = o.value("url").toString();

        if (url.isEmpty()) continue;

        QString login, password;

        const QJsonValue secretVal = o.value("secret");
        if (secretVal.isObject()) {
            const QJsonObject secret = secretVal.toObject();
            login = secret.value("login").toString();
            password = secret.value("password").toString();
        }

        creditem it;
        it.url = url;
        it.login = login;
        it.password = password;
        outCreds.push_back(it);
    }

    return true;
}
