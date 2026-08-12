#ifdef SERVER
/**
 *  KillFeed - turn off other mods' kill feeds, so a death is announced once rather than twice.
 *
 *  Every block here is guarded by the other mod's own define, so the addon still compiles and runs
 *  on a server where that mod is absent. Nothing is referenced unconditionally.
 *
 *  Deliberately flips each mod's *own* documented switch rather than overriding its classes: the
 *  intent is "leave it configured off", not "break it". Nothing here is persisted - it is an
 *  in-memory change applied after that mod has loaded its settings from disk, so an admin's own
 *  JSON is left exactly as they wrote it and removing this addon restores the previous behaviour.
 */
class KillFeedSuppress
{
    static void Apply()
    {
        KillFeedData settings = KillFeedConfig.GetConfig().GetSettings();
        if (!settings.suppress_other_killfeeds)
            return;

        SuppressExpansion();
    }

    /**
     *  DayZ-Expansion gates its whole kill feed on one setting, checked at both of its hooks in its
     *  own modded PlayerBase (EEKilled and EEHitBy). Clearing it is therefore complete - the module
     *  is simply never asked to report anything.
     *
     *  EnableKillFeed only exists when Expansion's kill feed addon is loaded, hence the define; the
     *  field is declared inside the same guard in ExpansionNotificationSettings.
     */
    private static void SuppressExpansion()
    {
#ifdef EXPANSIONMODKILLFEED
        ExpansionNotificationSettings notifications = GetExpansionSettings().GetNotification();
        if (!notifications)
            return;

        if (!notifications.EnableKillFeed)
            return;

        notifications.EnableKillFeed = false;
        KillFeedLog.Info("Disabled the DayZ-Expansion kill feed (EnableKillFeed = false, in memory only)");
#endif
    }
}
#endif
