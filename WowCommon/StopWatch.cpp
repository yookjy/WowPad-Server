#include "stdafx.h"
#include "VsUtils.h"


vs::utils::StopWatch::StopWatch(void)
{
	if(!::QueryPerformanceFrequency(&frequency_)) throw "Error with QueryPerformanceFrequency";
}


vs::utils::StopWatch::~StopWatch(void)
{
}

void vs::utils::StopWatch::Start()
{
	::QueryPerformanceCounter(&startTime_);
}

void vs::utils::StopWatch::Stop()
{
	::QueryPerformanceCounter(&stopTime_);
}

void vs::utils::StopWatch::Reset()
{
	startTime_.QuadPart = 0;
	stopTime_.QuadPart = 0;
}

LONGLONG vs::utils::StopWatch::MilliSeconds() const
{
	return (stopTime_.QuadPart - startTime_.QuadPart) * 1000 / frequency_.QuadPart; 
}

LONGLONG vs::utils::StopWatch::StartMilliSeconds() const
{
	return startTime_.QuadPart * 1000 / frequency_.QuadPart; 
}

LONGLONG vs::utils::StopWatch::StopMilliSeconds() const
{
	return stopTime_.QuadPart * 1000 / frequency_.QuadPart; 
}