/**
 *  Auto-Run - the public API. THIS IS THE ENTIRE CONTRACT with the host game.
 *
 *  The addon needs nothing from a host game to work: press the bind, hold a speed, press it again.
 *  The API exists for one situation only - a host game that drives
 *  HumanInputController.OverrideMovementSpeed itself, which is what freezing a player looks like.
 *  Auto-run re-asserts its own override every frame, so without a way to say "not now" the two would
 *  fight and the freeze would lose.
 *
 *  Every method is safe to call at any time, including before anything is initialised, so a host
 *  game never has to null-check.
 *
 *  Usage from the host game (guard every call site, so removing the addon still builds):
 *
 *      #ifdef VIGRID_AUTORUN
 *          VigridAutoRunAPI.SetAllowed(false);       // client, before disabling input
 *          VigridAutoRunAPI.CancelFor(player);       // server, ON THE LINE ABOVE the freeze
 *      #endif
 *
 *  The two halves are deliberately named APART rather than overloaded on their argument. The guards
 *  are mutually exclusive so one name would compile, but they do not mean the same thing - one
 *  suppresses the local player's key, the other drops the server's record for somebody else - and an
 *  unguarded 4_World or 5_Mission caller would compile against both and silently mean something
 *  different per side, which no build would catch. Party learned this one the hard way; see
 *  VigridPartyAPI.IsClientReady.
 */
class VigridAutoRunAPI
{
#ifdef SERVER

    /**
     *  Stop tracking this player, WITHOUT touching their input controller.
     *
     *  Call it on the line above whatever writes the movement override - order matters, cancel
     *  first and freeze second. See VigridAutoRunState for why the release is deliberately not done
     *  here.
     */
    static void CancelFor(PlayerBase player)
    {
        if (!player)
            return;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return;

        VigridAutoRunState.Forget(identity.GetPlainId());
    }

    //! Whether the server is currently forcing a movement speed on this player on auto-run's behalf.
    static bool IsHeldFor(PlayerBase player)
    {
        if (!player)
            return false;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return false;

        return VigridAutoRunState.IsHeld(identity.GetPlainId());
    }

#endif

#ifndef SERVER

    /**
     *  Allow or refuse auto-run for the local player. Refusing also cancels a hold in progress, so a
     *  host game needs no separate cancel call.
     *
     *  Idempotent, and not persisted - it is a live gate, not a setting.
     */
    static void SetAllowed(bool allowed)
    {
        VigridAutoRunClient.GetInstance().SetAllowed(allowed);
    }

    //! Whether the local player is holding a speed right now.
    static bool IsActive()
    {
        return VigridAutoRunClient.GetInstance().IsActive();
    }

#endif
}
