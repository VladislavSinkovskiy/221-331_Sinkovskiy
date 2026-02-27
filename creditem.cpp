#include "creditem.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QVector<creditem> creditem::loadFromJsonFile(const QString& path, QString* err)
{
    QVector<creditem> out;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = "Не удалось открыть файл: " + f.errorString();
        return out;
    }

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (err) *err = "Ошибка парсинга JSON: " + pe.errorString();
        return out;
    }

    const QJsonObject root = doc.object();
    const QJsonValue credsVal = root.value("creds");
    if (!credsVal.isArray()) {
        if (err) *err = "Неверный формат: поле 'creds' должно быть массивом.";
        return out;
    }

    const QJsonArray arr = credsVal.toArray();
    out.reserve(arr.size());

    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;

        const QJsonObject obj = v.toObject();
        const QString url = obj.value("url").toString();

        const QJsonObject secret = obj.value("secret").toObject();
        const QString login = secret.value("login").toString();
        const QString password = secret.value("password").toString();

        if (url.isEmpty()) continue;

        out.push_back(creditem(url, login, password));
    }

    return out;
}
