#ifndef CREDITEM_H
#define CREDITEM_H

#include <QString>
#include <QVector>

class creditem
{
public:
    creditem() = default;
    creditem(QString url, QString login, QString password)
        : url_(std::move(url)), login_(std::move(login)), password_(std::move(password)) {}

    const QString& url() const { return url_; }
    const QString& login() const { return login_; }
    const QString& password() const { return password_; }

    void setUrl(const QString& v) { url_ = v; }
    void setLogin(const QString& v) { login_ = v; }
    void setPassword(const QString& v) { password_ = v; }


    static QVector<creditem> loadFromJsonFile(const QString& path, QString* err = nullptr);

private:
    QString url_;
    QString login_;
    QString password_;
};

#endif
