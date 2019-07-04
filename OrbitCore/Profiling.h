//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------
#pragma once

#include "Platform.h"
#include "BaseTypes.h"

//-----------------------------------------------------------------------------
typedef uint64_t TickType;
extern TickType  GFrequency;
extern double    GPeriod;

//-----------------------------------------------------------------------------
void InitProfiling();

//-----------------------------------------------------------------------------
inline TickType OrbitTicks()
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return ticks.QuadPart;
#else
    timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return 1000000000ll * ts.tv_sec + ts.tv_nsec;
#endif
}

#ifdef WIN32
//-----------------------------------------------------------------------------
inline double MicroSecondsFromTicks( TickType a_Start, TickType a_End )
{
    return double(1000000*(a_End - a_Start))*GPeriod;
}

//-----------------------------------------------------------------------------
inline TickType TicksFromMicroseconds( double a_Micros )
{
    return (TickType)(GFrequency*a_Micros*0.000001);
}
#else
//-----------------------------------------------------------------------------
inline double MicroSecondsFromTicks( TickType a_Start, TickType a_End )
{
    return double((a_End - a_Start))*0.001;
}

//-----------------------------------------------------------------------------
inline TickType TicksFromMicroseconds( double a_Micros )
{
    return (TickType)(a_Micros*1000);
}
#endif
