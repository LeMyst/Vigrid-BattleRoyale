#ifdef SERVER
/**
 *  Battle Royale - wall-clock helper for anything that has to outlive the process.
 *
 *  Two different clocks, and they are not interchangeable:
 *
 *    - GetGame().GetTime() is milliseconds since the process started. Monotonic and cheap, and the
 *      right choice for anything session-scoped (the persistence write debounce, RPC cooldowns).
 *    - The leaderboard spans server restarts - this server restarts between every match - so
 *      pruning stale entries needs a real calendar reading. DayZ exposes only GetYearMonthDayUTC /
 *      GetHourMinuteSecondUTC, so this converts those into a plain integer hour count.
 *
 *  This deliberately duplicates Party's VigridPartyTime instead of calling it. The Battle Royale
 *  mod talks to the party addon only through VigridPartyAPI, so that renaming Party/config.cpp
 *  stays a one-rename kill switch; borrowing its clock would break that.
 */
class BattleRoyaleTime
{
    //--- Days since 1970-01-01 from a proleptic Gregorian date. Exact integer arithmetic
    //--- (Howard Hinnant's days_from_civil); the branches replace the ternaries the language does
    //--- not have. Only ever called with real calendar dates, so y is always positive.
    static int DaysFromCivil(int y, int m, int d)
    {
        int year = y;
        if (m <= 2)
            year = year - 1;

        int era = year / 400;
        int yoe = year - era * 400;

        int mp = m - 3;
        if (m <= 2)
            mp = m + 9;

        int doy = (153 * mp + 2) / 5 + d - 1;
        int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

        return era * 146097 + doe - 719468;
    }

    //--- Hours since the Unix epoch, UTC. Hour resolution is all leaderboard_ttl_days needs, and it
    //--- stays well inside int range (2026 is roughly 490k hours since the epoch).
    static int NowHours()
    {
        int year;
        int month;
        int day;
        GetYearMonthDayUTC(year, month, day);

        int hour;
        int minute;
        int second;
        GetHourMinuteSecondUTC(hour, minute, second);

        return DaysFromCivil(year, month, day) * 24 + hour;
    }

    //--- Seconds since the Unix epoch, UTC. Stored in the JSON purely so a human reading the file
    //--- can tell when it was written; the pruning logic uses NowHours().
    static int NowSeconds()
    {
        int year;
        int month;
        int day;
        GetYearMonthDayUTC(year, month, day);

        int hour;
        int minute;
        int second;
        GetHourMinuteSecondUTC(hour, minute, second);

        return DaysFromCivil(year, month, day) * 86400 + hour * 3600 + minute * 60 + second;
    }

    //--- Monotonic milliseconds since process start. Use for anything session-scoped.
    static int NowMs()
    {
        return GetGame().GetTime();
    }
}
#endif
