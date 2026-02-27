#pragma once
#include <QVector>
#include <QString>
#include "creditem.h"

bool loadCredsFromJson(const QString& path, QVector<creditem>& outCreds, QString* err);
