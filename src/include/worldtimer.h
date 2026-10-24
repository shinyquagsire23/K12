#ifndef CWORLDTIMER_H
#define CWORLDTIMER_H

#include "exostring.h"

// World clock and frame-snapshot timing.
struct CWorldTimer {
    int            m_nFixedRateNumerator;
    int            m_nFixedRateDenominator;
    int            m_nSnapshotScale;
    unsigned char  m_aReserved1[0x10-0x0c];
    long long      m_nSnapshotTime;
    long long      m_nLastSnapshotTime;
    long long      m_nLastHighResTime;
    int            m_bUseFixedRate;
    int            m_nPauseDay;
    int            m_nPauseTime;
    int            m_nCalendarDayOffset;
    int            m_nTimeOfDayOffset;
    unsigned char  m_nMinutesPerHour;
    unsigned char  m_aReserved2[3];
    int            m_nMillisecondsPerDay;
    unsigned int   m_nSecondsPerDay;

    CWorldTimer();
    ~CWorldTimer();
    void      SetSnapshotTime();
    long long GetSnapshotTime();
    void      ResetLastSnapshotTime() { m_nLastSnapshotTime = GetSnapshotTime(); }
    long long GetSnapshotTimeDifference();
    void      PauseWorldTimer();
    void      UnpauseWorldTimer();
    void      SetFixedSnapshotRate(int a, int b);
    void      SetSnapshotScale(float f);
    int       GetCalendarDayFromSeconds(float f);
    int       GetTimeOfDayFromSeconds(float f);
    int       ConvertToCalendarDay(unsigned int year, unsigned int month, unsigned int day);
    void      SetMinutesPerHour(unsigned char mph);
    void      ResetTimer(CWorldTimer* pSource);
    int       CompareWorldTimes(unsigned int day1, unsigned int tod1,
                                unsigned int day2, unsigned int tod2);
    int       ConvertToTimeOfDay(unsigned int hour, unsigned int minute,
                                 unsigned int second, unsigned int ms);
    unsigned int GetWorldTimeYear();
    unsigned int GetWorldTimeCalendarDay();
    unsigned int GetWorldTimeHour();
    unsigned int GetWorldTimeMinute();
    unsigned int GetWorldTimeSecond();
    unsigned int GetWorldTimeMillisecond();
    unsigned int GetWorldTimeTimeOfDay();
    unsigned int GetWorldTimeDay();
    unsigned int GetWorldTimeMonth();
    int GetTimeDifferenceFromWorldTime(unsigned int day, unsigned int tod);
    int  AddWorldTimes(unsigned int day1, unsigned int tod1, unsigned int day2,
                       unsigned int tod2, unsigned int* pDay, unsigned int* pTod);

    // Declared here and defined in the .cpp: an initializer visible in this header would fold the
    // value into the code and drop the load.
    static const unsigned int MILLISECONDS_IN_SECOND;
    static const unsigned int MILLISECONDS_IN_MINUTE;
    // The start-clock defaults a module seeds its clock from.
    static const int DEFAULT_YEAR;
    static const int DEFAULT_MONTH;
    static const int DEFAULT_DAY;
    static const int DEFAULT_HOUR;
    static const int HOURS_IN_DAY;
    int  SubtractWorldTimes(unsigned int day1, unsigned int tod1, unsigned int day2,
                            unsigned int tod2, unsigned int* pDay, unsigned int* pTod);
    void ConvertFromTimeOfDay(unsigned int tod, unsigned int* pHour, unsigned int* pMinute,
                              unsigned int* pSecond, unsigned int* pMs);
    void ConvertFromCalendarDay(unsigned int day, unsigned int* pYear,
                                unsigned int* pMonth, unsigned int* pDay);
    void GetWorldTime(unsigned int* pA, unsigned int* pB);
    void SetWorldTime(unsigned int day, unsigned int tod, int bUpdateSnapshot);
    void SetWorldTime(unsigned int year, unsigned int month, unsigned int day, unsigned int hour,
                       unsigned int minute, unsigned int second, unsigned int ms,
                       int bUpdateSnapshot);
    CExoString GetWorldTimeTimeOfDayString();
    CExoString GetWorldTimeCalendarDayString();
    CExoString TranslateWorldTimeToString(unsigned int day, unsigned int tod,
                                           CExoString& dayName, CExoString& monthName);
    void AdvanceToTime(int hours, int minutes, int seconds, int ms);
};

#endif
