#ifdef SERVER
/**
 *  Auto-Run - the server's record of who it is holding a movement override for.
 *
 *  ⚠️ THE RECORD IS THE POINT OF THIS CLASS, and it exists to stop one specific race.
 *
 *  A host mod that freezes players does it by writing the very same override - this repo's
 *  PlayerBase.DisableInput is OverrideMovementSpeed(ENABLED, 0). If a client's "auto-run off"
 *  message lands AFTER such a freeze, a handler that simply passed DISABLED would release the
 *  freeze and hand a frozen player their controls back, with nothing in any log to say so.
 *
 *  So a release is only ever performed for a uid this addon still holds a record for, and a host
 *  mod's Cancel drops the record WITHOUT touching the controller - the host is about to write its
 *  own override on the very next line, and clearing first would be undoing it in advance.
 *
 *  Keyed by SteamID64 rather than by entity, so a body that is replaced (respawn, admin tooling)
 *  cannot strand an entry pointing at a freed object.
 */
class VigridAutoRunState
{
    //! uid -> the speed currently forced on that player. Absent means this addon holds nothing.
    private static ref map<string, int> s_Held;

    private static map<string, int> Held()
    {
        if (!s_Held)
            s_Held = new map<string, int>();

        return s_Held;
    }

    static bool IsHeld(string uid)
    {
        return Held().Contains(uid);
    }

    /**
     *  Apply a speed, or release on VIGRID_AUTORUN_SPEED_OFF.
     *
     *  The release half is the guarded one: it does nothing at all unless this addon is what put the
     *  override there.
     */
    static void Apply(PlayerBase player, string uid, int speed)
    {
        if (!player)
            return;
        if (uid == "")
            return;

        HumanInputController hic = player.GetInputController();
        if (!hic)
            return;

        if (speed == VIGRID_AUTORUN_SPEED_OFF)
        {
            if (!Held().Contains(uid))
            {
                VigridAutoRunLog.Debug("Release ignored for " + uid + ": nothing held here");
                return;
            }

            Held().Remove(uid);
            hic.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
            hic.OverrideMovementAngle(HumanInputControllerOverrideType.DISABLED, 0);

            VigridAutoRunLog.Debug("Released " + uid);
            return;
        }

        Held().Set(uid, speed);
        hic.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, speed);
        hic.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, VIGRID_AUTORUN_MOVEMENT_ANGLE);

        VigridAutoRunLog.Debug("Holding speed " + speed + " for " + uid);
    }

    /**
     *  Forget a player without touching their controller.
     *
     *  For a host mod that is about to drive the override itself. After this, a late "auto-run off"
     *  from that client is a no-op rather than an unfreeze.
     */
    static void Forget(string uid)
    {
        if (uid == "")
            return;
        if (!Held().Contains(uid))
            return;

        Held().Remove(uid);

        VigridAutoRunLog.Debug("Forgot " + uid + " at the host mod's request");
    }

    /**
     *  Release and forget. For a disconnect: a client that drops mid auto-run would otherwise leave
     *  a combat-logout body sprinting across the map under a held override nobody is left to cancel.
     */
    static void Clear(PlayerBase player, string uid)
    {
        if (uid == "")
            return;
        if (!Held().Contains(uid))
            return;

        Held().Remove(uid);

        if (!player)
            return;

        HumanInputController hic = player.GetInputController();
        if (!hic)
            return;

        hic.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
        hic.OverrideMovementAngle(HumanInputControllerOverrideType.DISABLED, 0);

        VigridAutoRunLog.Debug("Cleared " + uid + " on disconnect");
    }
}
#endif
