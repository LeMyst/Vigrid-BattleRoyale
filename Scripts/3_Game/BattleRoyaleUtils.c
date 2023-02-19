class BattleRoyaleUtils extends Managed {
    static const int NONE = 0;
    static const int WARN = 1;
    static const int INFO = 2;
    static const int DEBUG = 3;
    static const int TRACE = 4;

    static void LogMessage(string message) {
        Print("[DayZ-BattleRoyale] " + message);
    }

    static void LogServerMessage(int level, string message) {
        if(CheckLogLevel(level)) {
            Print("[DayZ-BattleRoyale][" + level + "] " + message);
        }
    }

    static void Warn(string message) {
        LogServerMessage(WARN, message);
    }

    static void Info(string message) {
        LogServerMessage(INFO, message);
    }

    static void Debug(string message) {
        LogServerMessage(DEBUG, message);
    }

    static void Trace(string message) {
        LogServerMessage(TRACE, message);
    }

    static bool CheckLogLevel(int level) {
        return BATTLEROYALE_LOG_LEVEL >= level;
    }
}
