class BattleRoyaleUtils: Managed
{
    // Move to enum ?
    static const int NONE = 0;
    static const int WARN = 1;
    static const int INFO = 2;
    static const int DEBUG = 3;
    static const int TRACE = 4;

    static void LogMessage(string message)
    {
        BattleRoyaleUtils.Trace("[DayZ-BattleRoyale] " + message);
    }

    static void LogServerMessage(int level, string message)
    {
        if(CheckLogLevel(level))
        {
        	if (level == NONE) {
        		Error("[DayZ-BattleRoyale] " + message);
        	} else {
            	Print("[DayZ-BattleRoyale][" + level + "] " + message);
            }
        }
    }

    static void Error(string message)
    {
        LogServerMessage(NONE, message);
    }

    static void Warn(string message)
    {
        LogServerMessage(WARN, message);
    }

    static void Info(string message)
    {
        LogServerMessage(INFO, message);
    }

    static void Debug(string message)
    {
        LogServerMessage(DEBUG, message);
    }

    static void Trace(string message)
    {
        LogServerMessage(TRACE, message);
    }

    static void Trace(vector v)
	{
		LogServerMessage(TRACE, v.ToString());
	}

	static void Trace(float f)
	{
		LogServerMessage(TRACE, f.ToString());
	}

    static bool CheckLogLevel(int level)
    {
    	// TODO: Add get log level from config file (if exists)
        return BATTLEROYALE_LOG_LEVEL >= level;
    }

    static string GetDateTime(string dateTimeFormat = "%1.%2.%3 %4:%5:%6")
    {
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;

        GetYearMonthDay(year, month, day);
        GetHourMinuteSecond(hour, minute, second);
        return string.Format(dateTimeFormat, day.ToStringLen(2), month.ToStringLen(2), year.ToStringLen(4), hour.ToStringLen(2), minute.ToStringLen(2), second.ToStringLen(2));
    }

    static string GetTime(bool withSeconds = false)
    {
        string dateTimeFormat = "";
        int hour = 0;
        int minute = 0;
        int second = 0;
        GetHourMinuteSecond(hour, minute, second);

        if (withSeconds)
        {
            dateTimeFormat = "%1:%2:%3";
            return string.Format(dateTimeFormat, hour.ToStringLen(2), minute.ToStringLen(2), second.ToStringLen(2));
        }
        else
        {
            dateTimeFormat = "%1:%2";
            return string.Format(dateTimeFormat, hour.ToStringLen(2), minute.ToStringLen(2));
        }
    }
}
