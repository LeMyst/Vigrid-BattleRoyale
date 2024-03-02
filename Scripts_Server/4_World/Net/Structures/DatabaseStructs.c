class ServerData
{
    string _id;
    string name;
    string connection;
    string query_port;
    string version;
    int last_started;
    ref array<string> matches;
    string region;
    bool locked;

    string GetIP()
    {
        TStringArray parts = new TStringArray;
        connection.Split(":",parts);

        return parts.Get(0);
    }

    int GetPort()
    {
        TStringArray parts = new TStringArray;
        connection.Split(":",parts);

        return parts.Get(1).ToInt();
    }

    bool CanConnect()
    {
        return !locked;
    }

    bool IsMatchingVersion()
    {
        // Avoid a potential dayz backend fuckery
        if ( version == BATTLEROYALE_VERSION )
            return true;

        return false;
    }
}
