#ifndef SECURITY_STATE_H
#define SECURITY_STATE_H

#include <QString>

namespace SecurityState {

bool isAttackDetected();
QString attackMessage();

void setAttackDetected(const QString& message);
void reset();

}

#endif
