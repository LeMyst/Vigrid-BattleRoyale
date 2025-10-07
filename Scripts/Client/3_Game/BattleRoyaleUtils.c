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
        	int hour;
        	int minute;
        	int second;

        	GetHourMinuteSecond(hour, minute, second);

			string msg;
        	if (level == NONE) {
        		msg = hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2) + " [DayZ-BattleRoyale] " + message;
        		Error2("", string.ToString(msg));
        	} else {
        		msg = hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2) + " [DayZ-BattleRoyale][" + level + "] " + message;
	        	PrintFormat("%1:%2:%3 [DayZ-BattleRoyale][%4] %5", hour.ToStringLen(2), minute.ToStringLen(2), second.ToStringLen(2), level, message);
            }

			string chat_color = "default";
			if (level == WARN) {
				chat_color = "colorFriendly";
			} else if (level == INFO) {
				chat_color = "colorStatusChannel";
			} else if (level == DEBUG) {
				chat_color = "colorAction";
			} else if (level == TRACE) {
				chat_color = "colorImportant";
			}

#ifdef SERVER
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ChatLog", new Param2<string, string>(msg, chat_color), true);
#else
			if ( GetGame() )
				GetGame().Chat("C:" + msg, chat_color);
#endif
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
		// Check command line params
		if (IsCLIParam("br-trace")) return (TRACE >= level);
		if (IsCLIParam("br-debug")) return (DEBUG >= level);
		if (IsCLIParam("br-info")) return (INFO >= level);
		if (IsCLIParam("br-warn")) return (WARN >= level);
		if (IsCLIParam("br-none")) return false;  // Disable all logging

#ifdef SERVER
		// Check server config
		int config_br_log_level = GetGame().ServerConfigGetInt("BRLogLevel");
		if (config_br_log_level > 0)
		{
			return (config_br_log_level >= level);
		}
		// config_br_log_level == 0 can mean that the config value is not set
		// In that case we use the default log level defined by BATTLEROYALE_LOG_LEVEL
		// Check negative value to disable all logging
		if (config_br_log_level < 0)
		{
			return false;  // Disable all logging
		}
#endif

		// Default log level
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
