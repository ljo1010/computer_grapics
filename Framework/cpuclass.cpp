///////////////////////////////////////////////////////////////////////////////
// Filename: cpuclass.cpp
///////////////////////////////////////////////////////////////////////////////
#include "cpuclass.h"


CpuClass::CpuClass()
{
}


CpuClass::CpuClass(const CpuClass& other)
{
}


CpuClass::~CpuClass()
{
}


#include <pdh.h>
#include <pdhmsg.h>
// …

void CpuClass::Initialize()
{
    PDH_STATUS status;

    m_canReadCpu = true;
    status = PdhOpenQuery(NULL, 0, &m_queryHandle);
    if (status != ERROR_SUCCESS) {
        m_canReadCpu = false;
        return;
    }

    status = PdhAddEnglishCounterW(
        m_queryHandle,
        L"\\Processor(_Total)\\% Processor Time",
        0,
        &m_counterHandle
    );

    if (status != ERROR_SUCCESS) {
        status = PdhAddCounterW(
            m_queryHandle,
            L"\\프로세서(_전체)\\프로세서 시간 %",
            0,
            &m_counterHandle
        );
    }

    if (status != ERROR_SUCCESS) {
        m_canReadCpu = false;
    }

    m_lastSampleTime = GetTickCount();
    m_cpuUsage = 0;
}


void CpuClass::Shutdown()
{
	if (m_canReadCpu)
	{
		PdhCloseQuery(m_queryHandle);
	}

	return;
}


void CpuClass::Frame()
{
	PDH_FMT_COUNTERVALUE value;

	if (m_canReadCpu)
	{
		if ((m_lastSampleTime + 1000) < GetTickCount())
		{
			m_lastSampleTime = GetTickCount();

			PdhCollectQueryData(m_queryHandle);

			PdhGetFormattedCounterValue(m_counterHandle, PDH_FMT_LONG, NULL, &value);

			m_cpuUsage = value.longValue;
		}
	}

	return;
}


int CpuClass::GetCpuPercentage()
{
	int usage;

	if (m_canReadCpu)
	{
		usage = (int)m_cpuUsage;
	}
	else
	{
		usage = 0;
	}

	return usage;
}