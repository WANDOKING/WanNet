#pragma once

#include <cstdint>

extern uint32_t g_totalConnectedClientCount;

extern uint64_t g_connectTotalCount;
extern uint64_t g_connectFailCount;

extern uint64_t g_downCount;
extern uint64_t g_echoErrorCount;

extern uint64_t g_rttSum;
extern uint64_t g_rttCount;

extern uint32_t g_echoSendCount;
extern uint32_t g_echoRecvCount;