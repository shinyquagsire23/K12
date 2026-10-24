#include "k2stdint.h"
#include "worldtimer.h"

#include "exobase.h"

const int CWorldTimer::DEFAULT_YEAR = 1340;
const int CWorldTimer::DEFAULT_MONTH = 6;
const int CWorldTimer::DEFAULT_DAY = 1;
const int CWorldTimer::DEFAULT_HOUR = 23;
const int CWorldTimer::HOURS_IN_DAY = 24;
const unsigned int CWorldTimer::MILLISECONDS_IN_SECOND = 1000;
const unsigned int CWorldTimer::MILLISECONDS_IN_MINUTE = 60000;

// The game calendar: 28-day months, 12 to the year, and the year a far-out day count clamps to.
enum {
    DAYS_PER_MONTH = 28,
    MONTHS_PER_YEAR = 12,
    DAYS_PER_YEAR = DAYS_PER_MONTH * MONTHS_PER_YEAR,
    MAX_YEAR = 1340,
};

CWorldTimer::CWorldTimer()
{
    m_nFixedRateNumerator = 0;
    m_nMinutesPerHour = 5;
    m_nSnapshotScale = 100;
    m_nMillisecondsPerDay = 7200000;
    m_nSecondsPerDay = 7200;
    long long t1 = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    m_nSnapshotTime = t1;
    long long t2 = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    m_nLastSnapshotTime = m_nSnapshotTime;
    m_nLastHighResTime = t2;
    SetWorldTime(1340, 6, 1, 23, 59, 56, 0, 0);
    m_bUseFixedRate = 0;
}

long long CWorldTimer::GetSnapshotTime()
{
    return m_nSnapshotTime;
}

long long CWorldTimer::GetSnapshotTimeDifference()
{
    if (m_bUseFixedRate != 0)
        return 0;
    return m_nSnapshotTime - m_nLastSnapshotTime;
}

void CWorldTimer::SetFixedSnapshotRate(int a, int b)
{
    m_nSnapshotTime = 0;
    m_nLastSnapshotTime = 0;
    m_nFixedRateNumerator = a;
    m_nFixedRateDenominator = b;
}

void CWorldTimer::SetSnapshotScale(float f)
{
    m_nSnapshotScale = static_cast<int>(f * 100.0f);
}

int CWorldTimer::GetCalendarDayFromSeconds(float f)
{
    float perDay = static_cast<float>(m_nSecondsPerDay);
    int nDay = 0;
    while (f >= perDay) {
        f -= perDay;
        nDay++;
    }
    return nDay;
}

CWorldTimer::~CWorldTimer()
{
}

int CWorldTimer::GetTimeOfDayFromSeconds(float f)
{
    if (f < 0.0f)
        return 0;
    float perDay = static_cast<float>(m_nSecondsPerDay);
    while (f >= perDay)
        f -= perDay;
    return static_cast<unsigned int>(f * 1000.0f);
}

void CWorldTimer::SetMinutesPerHour(unsigned char mph)
{
    if (mph == 0)
        mph = 5;
    m_nMinutesPerHour = mph;
    m_nMillisecondsPerDay = mph * 1440000;
    m_nSecondsPerDay = mph * 1440;
}

void CWorldTimer::ResetTimer(CWorldTimer* pSource)
{
    m_nFixedRateNumerator   = pSource->m_nFixedRateNumerator;
    m_nFixedRateDenominator = pSource->m_nFixedRateDenominator;
    m_nSnapshotTime         = pSource->m_nSnapshotTime;
    m_nLastSnapshotTime     = pSource->m_nLastSnapshotTime;
    m_bUseFixedRate         = pSource->m_bUseFixedRate;
    m_nPauseDay                 = pSource->m_nPauseDay;
    m_nPauseTime                 = pSource->m_nPauseTime;
    m_nCalendarDayOffset                 = pSource->m_nCalendarDayOffset;
    m_nTimeOfDayOffset                 = pSource->m_nTimeOfDayOffset;
    m_nMinutesPerHour       = pSource->m_nMinutesPerHour;
    m_nMillisecondsPerDay                 = pSource->m_nMillisecondsPerDay;
    m_nSecondsPerDay        = pSource->m_nSecondsPerDay;
    m_nSnapshotScale        = 100;
}

// Compares two (calendar-day, time-of-day) pairs. Returns -2 if either time of day is out of range,
// otherwise -1, 0 or 1.
int CWorldTimer::CompareWorldTimes(unsigned int day1, unsigned int tod1,
                                   unsigned int day2, unsigned int tod2)
{
    if (static_cast<unsigned int>(m_nMillisecondsPerDay) <= tod1)
        return -2;
    if (static_cast<unsigned int>(m_nMillisecondsPerDay) <= tod2)
        return -2;
    if (day1 == day2) {
        if (tod1 <= tod2)
            return (tod1 < tod2) ? -1 : 0;
        return 1;
    }
    return (day1 <= day2) ? -1 : 1;
}

// Packs (hour, minute, second, ms) into a time-of-day count in milliseconds, scaled by the game's
// minutes per hour. Out-of-range fields clamp to a fixed maximum.
int CWorldTimer::ConvertToTimeOfDay(unsigned int hour, unsigned int minute,
                                    unsigned int second, unsigned int ms)
{
    unsigned int nMinutesPerHour = m_nMinutesPerHour;
    int nHour   = (hour   > 23) ? 23 * 60000 : static_cast<int>(hour * 60000);
    int nMinute = (minute > 59) ? 59 * 60000 : static_cast<int>(minute * 60000);
    int nSecond = (second > 59) ? 56000    : static_cast<int>(second * 1000);
    return nMinute + ms + nSecond + nHour * nMinutesPerHour;
}

// Unpacking the snapshot into a (day, time-of-day) pair is shared by this whole family.
unsigned int CWorldTimer::GetWorldTimeYear()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    return (nDay >> 19 > 20) ? MAX_YEAR : nDay / DAYS_PER_YEAR;
}

unsigned int CWorldTimer::GetWorldTimeCalendarDay()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    return nDay;
}

unsigned int CWorldTimer::GetWorldTimeHour()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    unsigned int nMinutesTotal = nTod / 60000;
    return nMinutesTotal / m_nMinutesPerHour;
}

unsigned int CWorldTimer::GetWorldTimeMinute()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    unsigned int nMinutesTotal = nTod / 60000;
    unsigned int nHours = nMinutesTotal / m_nMinutesPerHour;
    return nMinutesTotal - nHours * m_nMinutesPerHour;
}

unsigned int CWorldTimer::GetWorldTimeSecond()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    return (nTod / 1000) % 60;
}

unsigned int CWorldTimer::GetWorldTimeMillisecond()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    return nTod % 1000;
}

unsigned int CWorldTimer::GetWorldTimeTimeOfDay()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    return nTod;
}

CExoString CWorldTimer::GetWorldTimeTimeOfDayString()
{
    CExoString result;
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    unsigned int nHour, nMinute, nSecond, nMs;
    ConvertFromTimeOfDay(nTod, &nHour, &nMinute, &nSecond, &nMs);
    result.Format(const_cast<char*>("%02d:%02d:%02d.%03d"), nHour, nMinute, nSecond, nMs);
    return result;
}

unsigned int CWorldTimer::GetWorldTimeDay()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    unsigned int nWeeks = (nDay >> 2) / 7;
    return nDay - nWeeks * DAYS_PER_MONTH + 1;
}

unsigned int CWorldTimer::GetWorldTimeMonth()
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    unsigned int nWeeks = (nDay >> 2) / 7;
    return nWeeks % 12 + 1;
}

CExoString CWorldTimer::GetWorldTimeCalendarDayString()
{
    CExoString result;
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    unsigned int nYear, nMonth, nDayOfMonth;
    ConvertFromCalendarDay(nDay, &nYear, &nMonth, &nDayOfMonth);
    result.Format(const_cast<char*>("%02d/%02d/%04d"), nDayOfMonth, nMonth, nYear);
    return result;
}

CExoString CWorldTimer::TranslateWorldTimeToString(unsigned int day, unsigned int tod,
                                                    CExoString& dayName, CExoString& monthName)
{
    CExoString result;
    unsigned int nYear, nMonth, nDayOfMonth;
    unsigned int nHour, nMinute, nSecond, nMs;
    ConvertFromCalendarDay(day, &nYear, &nMonth, &nDayOfMonth);
    ConvertFromTimeOfDay(tod, &nHour, &nMinute, &nSecond, &nMs);
    result.Format(const_cast<char*>("%s %02d, %s %02d, %04d  [%02d:%02d]"),
                  dayName.CStr(), nDayOfMonth, monthName.CStr(), nMonth, nYear, nHour, nMinute);
    return result;
}

int CWorldTimer::GetTimeDifferenceFromWorldTime(unsigned int day, unsigned int tod)
{
    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);
    unsigned int nDiffDay = 0;
    unsigned int nDiffTod = 0;
    SubtractWorldTimes(nDay, nTod, day, tod, &nDiffDay, &nDiffTod);
    return nDiffTod;
}

int CWorldTimer::AddWorldTimes(unsigned int day1, unsigned int tod1, unsigned int day2,
                               unsigned int tod2, unsigned int* pDay, unsigned int* pTod)
{
    int nResult = -2;
    if (tod1 < static_cast<unsigned int>(m_nMillisecondsPerDay) && tod2 < static_cast<unsigned int>(m_nMillisecondsPerDay)) {
        *pDay = day2 + day1;
        *pTod = tod2 + tod1;
        if (tod2 + tod1 < static_cast<unsigned int>(m_nMillisecondsPerDay))
            return 0;
        nResult = 0;
        *pDay = *pDay + 1;
        unsigned int nSpan = m_nMillisecondsPerDay;
        *pTod = *pTod - nSpan;
    }
    return nResult;
}

int CWorldTimer::SubtractWorldTimes(unsigned int day1, unsigned int tod1, unsigned int day2,
                                    unsigned int tod2, unsigned int* pDay, unsigned int* pTod)
{
    if (tod1 < static_cast<unsigned int>(m_nMillisecondsPerDay) && tod2 < static_cast<unsigned int>(m_nMillisecondsPerDay)) {
        if (day1 == day2) {
            if (tod1 < tod2)
                return -2;
        } else if (day1 <= day2) {
            return -2;
        }
    }
    *pDay = day1 - day2;
    *pTod = tod1 - tod2;
    if (tod1 - tod2 < static_cast<unsigned int>(m_nMillisecondsPerDay))
        return 0;
    *pDay = *pDay - 1;
    *pTod = m_nMillisecondsPerDay + *pTod;
    return 0;
}

void CWorldTimer::ConvertFromTimeOfDay(unsigned int tod, unsigned int* pHour, unsigned int* pMinute,
                                       unsigned int* pSecond, unsigned int* pMs)
{
    *pMs = tod % 1000;
    *pSecond = (tod / 1000) % 60;
    unsigned char nMinutesPerHour = m_nMinutesPerHour;
    unsigned int nMinutesTotal = tod / 60000;
    unsigned int nHours = nMinutesTotal / nMinutesPerHour;
    *pMinute = nMinutesTotal - nHours * nMinutesPerHour;
    *pHour = nHours;
}

void CWorldTimer::ConvertFromCalendarDay(unsigned int day, unsigned int* pYear,
                                         unsigned int* pMonth, unsigned int* pDay)
{
    unsigned int nWeeks = (day >> 2) / 7;
    *pDay = day % DAYS_PER_MONTH + 1;
    *pMonth = nWeeks % 12 + 1;
    *pYear = (day >> 19 > 20) ? MAX_YEAR : day / DAYS_PER_YEAR;
}

int CWorldTimer::ConvertToCalendarDay(unsigned int year, unsigned int month, unsigned int day)
{
    int base = (year > 32000) ? 450240 : static_cast<int>(year * 336);
    if (month - 1 >= 12)
        base += 140;
    else
        base = month * 28 + base - 28;
    unsigned int d = day - 1;
    return base + ((d > 27) ? 0 : static_cast<int>(d));
}

// Latches the running offsets into the fixed-rate snapshot and sets m_bUseFixedRate. Inverse of
// UnpauseWorldTimer.
void CWorldTimer::PauseWorldTimer()
{
    if (m_bUseFixedRate != 0)
        return;

    uint64_t nTotal = static_cast<uint64_t>(m_nSnapshotTime) / 1000;
    unsigned int nSpan = m_nMillisecondsPerDay;
    int nDays = static_cast<int>(nTotal / nSpan);
    int nDay = m_nCalendarDayOffset + nDays;
    unsigned int nTod = m_nTimeOfDayOffset + static_cast<unsigned int>(nTotal % nSpan);
    while (~nSpan < nTod) {
        nDay--;
        nTod += nSpan;
    }
    while (nSpan <= nTod) {
        nDay++;
        nTod -= nSpan;
    }
    m_bUseFixedRate = 1;
    m_nPauseDay = nDay;
    m_nPauseTime = nTod;
}

void CWorldTimer::UnpauseWorldTimer()
{
    if (m_bUseFixedRate != 1)
        return;

    SetWorldTime(m_nPauseDay, m_nPauseTime, 0);
    m_bUseFixedRate = 0;
}

// Advances the snapshot time by one tick: in free-running mode by the real elapsed timer delta,
// scaled by m_nSnapshotScale / 100; in fixed-rate mode by a constant derived from
// m_nFixedRateDenominator instead of the real clock.
void CWorldTimer::SetSnapshotTime()
{
    m_nLastSnapshotTime = m_nSnapshotTime;
    if (m_nFixedRateNumerator == 0) {
        long long t = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        m_nSnapshotTime += static_cast<long long>(static_cast<unsigned long long>((t - m_nLastHighResTime) * static_cast<long long>(m_nSnapshotScale)) / 100);
        m_nLastHighResTime = t;
    } else {
        int nRate = 1000000 / m_nFixedRateDenominator;
        m_nSnapshotTime += static_cast<long long>(m_nSnapshotScale * nRate) / 100;
    }
}

// If bUpdateSnapshot is set, first advances the snapshot clock; then re-derives the running offsets
// from the snapshot against the day and time of day, the inverse of the unpacking the
// GetWorldTime family does. Division by zero yields zero here, so neither divisor needs a guard.
void CWorldTimer::SetWorldTime(unsigned int day, unsigned int tod, int bUpdateSnapshot)
{
    if (bUpdateSnapshot == 1)
        SetSnapshotTime();
    uint64_t nTotal = static_cast<uint64_t>(m_nSnapshotTime) / 1000;
    unsigned int nSpan = m_nMillisecondsPerDay;
    m_nCalendarDayOffset = day - static_cast<unsigned int>(nTotal / nSpan);  // no divisor guard needed: division by zero yields zero
    m_nTimeOfDayOffset = tod - static_cast<unsigned int>(nTotal % nSpan);
}

// The (year, month, day, hour, minute, second, ms) overload converts both parts, then runs the same
// snapshot update and offset tail as the overload above.
void CWorldTimer::SetWorldTime(unsigned int year, unsigned int month, unsigned int day,
                                unsigned int hour, unsigned int minute, unsigned int second,
                                unsigned int ms, int bUpdateSnapshot)
{
    int nCalDay = ConvertToCalendarDay(year, month, day);
    int nTod = ConvertToTimeOfDay(hour, minute, second, ms);

    if (bUpdateSnapshot == 1)
        SetSnapshotTime();
    uint64_t nTotal = static_cast<uint64_t>(m_nSnapshotTime) / 1000;
    unsigned int nSpan = m_nMillisecondsPerDay;
    m_nCalendarDayOffset = nCalDay - static_cast<unsigned int>(nTotal / nSpan);  // division by zero yields zero
    m_nTimeOfDayOffset = nTod - static_cast<unsigned int>(nTotal % nSpan);
}

void CWorldTimer::GetWorldTime(unsigned int* pDay, unsigned int* pTod)
{
    unsigned int tod;
    if (m_bUseFixedRate == 1) {
        *pDay = m_nPauseDay;
        tod = m_nPauseTime;
    } else {
        uint64_t nTotal = static_cast<uint64_t>(m_nSnapshotTime) / 1000;
        unsigned int nSpan = m_nMillisecondsPerDay;
        unsigned int nRem = static_cast<unsigned int>(nTotal % nSpan);
        *pDay = m_nCalendarDayOffset + static_cast<unsigned int>(nTotal / nSpan);
        tod = m_nTimeOfDayOffset + nRem;
    }
    *pTod = tod;
    while (~static_cast<unsigned int>(m_nMillisecondsPerDay) < *pTod) {
        *pDay = *pDay - 1;
        *pTod = m_nMillisecondsPerDay + *pTod;
    }
    while (*pTod >= static_cast<unsigned int>(m_nMillisecondsPerDay)) {
        *pDay = *pDay + 1;
        unsigned int nSpan = m_nMillisecondsPerDay;
        *pTod = *pTod - nSpan;
    }
}

// Advances the world clock to the given time of day, rolling into the next day when that time has
// already passed: negative components bail out, the fields carry up from milliseconds, and the date
// carries days and months, bailing past year 32000.
//
// totalHours is signed while its day carry is unsigned: a total past the int range leaves a negative
// remainder, which the unsigned clamp turns into the fixed maximum.
void CWorldTimer::AdvanceToTime(int hours, int minutes, int seconds, int ms)
{
    if ((minutes | hours | seconds | ms) < 0)
        return;

    unsigned int totalSeconds = static_cast<unsigned int>(ms) / 1000 + seconds;
    unsigned int totalMinutes = totalSeconds / 60 + minutes;
    // Signed, because its remainder is a signed division, while the day carry and the range gate
    // are unsigned.
    int totalHours = totalMinutes / 60 + hours;
    unsigned int rSeconds = static_cast<int>(totalSeconds) % 60;
    unsigned int rMinutes = static_cast<int>(totalMinutes) % 60;
    unsigned int rHours = totalHours % 24;
    int rMs = ms % 1000;
    unsigned int nDaysCarry = static_cast<unsigned int>(totalHours) / 24;

    unsigned int nDay, nTod;
    GetWorldTime(&nDay, &nTod);

    unsigned int nYear, nMonth, nDayOfMonth;
    ConvertFromCalendarDay(nDay, &nYear, &nMonth, &nDayOfMonth);

    if (static_cast<unsigned int>(totalHours) < 24) {
        unsigned int nTargetTod = ConvertToTimeOfDay(rHours, rMinutes, rSeconds, rMs);
        if (CompareWorldTimes(nDay, nTod, nDay, nTargetTod) != 1)
            goto finalize;
        nDaysCarry = 1;
    }

    {
        unsigned int nSum = nDaysCarry + nDayOfMonth;
        unsigned int nMonthCarry = nSum / (DAYS_PER_MONTH + 1);
        nMonth = nMonthCarry + nMonth;
        unsigned int nYearCarry = nMonth / (MONTHS_PER_YEAR + 1);
        nYear = nYearCarry + nYear;
        if (nYear > 32000)
            return;
        nDayOfMonth = nSum % (DAYS_PER_MONTH + 1);
        nMonth = nMonth % (MONTHS_PER_YEAR + 1);
    }

finalize:
    SetWorldTime(nYear, nMonth, nDayOfMonth, rHours, rMinutes, rSeconds, rMs, 0);
}
