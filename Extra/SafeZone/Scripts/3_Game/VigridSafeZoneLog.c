/**
 *  SafeZone - logging. No guard: compiles on both client and server.
 *
 *  A deliberate near-duplicate of the host mod's logger. The discipline rule keeps this addon free
 *  of BattleRoyale* symbols, so it carries its own. It also carries its own CLI flags and its own
 *  serverDZ.cfg key, so the addons' verbosity can be tuned independently.
 *
 *  Runtime verbosity, highest priority first:
 *    -safezone-trace | -safezone-debug | -safezone-info | -safezone-warn | -safezone-none    (CLI)
 *    SafeZoneLogLevel = 1..4 in serverDZ.cfg, negative to silence               (server only)
 *    VIGRID_SAFEZONE_LOG_LEVEL                                                  (compile default)
 */
class VigridSafeZoneLog
{
    static const int NONE = 0;
    static const int WARN = 1;
    static const int INFO = 2;
    static const int DEBUG = 3;
    static const int TRACE = 4;

    static void LogMessage(int level, string message)
    {
        if (!CheckLogLevel(level))
            return;

        int hour;
        int minute;
        int second;
        GetHourMinuteSecond(hour, minute, second);

        string stamp = hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2);

        if (level == NONE)
        {
            Error2("", stamp + " [SafeZone] " + message);
            return;
        }

        PrintFormat("%1 [SafeZone][%2] %3", stamp, level, message);
    }

    static void Error(string message)
    {
        LogMessage(NONE, message);
    }

    static void Warn(string message)
    {
        LogMessage(WARN, message);
    }

    static void Info(string message)
    {
        LogMessage(INFO, message);
    }

    static void Debug(string message)
    {
        LogMessage(DEBUG, message);
    }

    static void Trace(string message)
    {
        LogMessage(TRACE, message);
    }

    static bool CheckLogLevel(int level)
    {
        //--- Command line wins.
        if (IsCLIParam("safezone-none"))
            return false;
        if (IsCLIParam("safezone-trace"))
            return TRACE >= level;
        if (IsCLIParam("safezone-debug"))
            return DEBUG >= level;
        if (IsCLIParam("safezone-info"))
            return INFO >= level;
        if (IsCLIParam("safezone-warn"))
            return WARN >= level;

#ifdef SERVER
        //--- Then serverDZ.cfg. 0 is indistinguishable from "key absent", so it falls through to
        //--- the compile-time default; a negative value silences the addon outright.
        int config_level = GetGame().ServerConfigGetInt("SafeZoneLogLevel");
        if (config_level > 0)
            return config_level >= level;
        if (config_level < 0)
            return false;
#endif

        return VIGRID_SAFEZONE_LOG_LEVEL >= level;
    }
}
