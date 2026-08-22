/**
 *  EarPlugs - logging. No guard: compiles on both client and server.
 *
 *  A deliberate near-duplicate of the host mod's logger, and of SafeZone's and Map's. The discipline
 *  rule keeps this addon free of BattleRoyale* symbols, so it carries its own, with its own CLI
 *  flags so its verbosity can be tuned independently.
 *
 *  Runtime verbosity, highest priority first:
 *    -earplugs-trace | -earplugs-debug | -earplugs-info | -earplugs-warn | -earplugs-none    (CLI)
 *    VIGRID_EARPLUGS_LOG_LEVEL                                                (compile default)
 *
 *  ⚠️ NO serverDZ.cfg KEY HERE, unlike SafeZone and Map, and its absence is deliberate rather than
 *  an omission. This addon runs only on clients, and GetGame().ServerConfigGetInt() returns 0 on a
 *  client for EVERY key - measured - so a serverDZ.cfg branch would be dead code that looks like a
 *  supported way to configure the thing. The CLI flags are the whole runtime surface.
 */
class VigridEarPlugsLog
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
            Error2("", stamp + " [EarPlugs] " + message);
            return;
        }

        PrintFormat("%1 [EarPlugs][%2] %3", stamp, level, message);
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
        if (IsCLIParam("earplugs-none"))
            return false;
        if (IsCLIParam("earplugs-trace"))
            return TRACE >= level;
        if (IsCLIParam("earplugs-debug"))
            return DEBUG >= level;
        if (IsCLIParam("earplugs-info"))
            return INFO >= level;
        if (IsCLIParam("earplugs-warn"))
            return WARN >= level;

        return VIGRID_EARPLUGS_LOG_LEVEL >= level;
    }
}
