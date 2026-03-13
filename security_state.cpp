#include "security_state.h"

namespace {
bool g_attackDetected = false;
QString g_attackMessage;
}

namespace SecurityState {

bool isAttackDetected()
{
    return g_attackDetected;
}

QString attackMessage()
{
    return g_attackMessage;
}

void setAttackDetected(const QString& message)
{
    g_attackDetected = true;
    g_attackMessage = message;
}

void reset()
{
    g_attackDetected = false;
    g_attackMessage.clear();
}

}
