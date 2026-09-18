#pragma once
#include "can.h"

void canOutputsSend(CanCycle cycle);
void sendTriggeredCanMessage(size_t index = 0);