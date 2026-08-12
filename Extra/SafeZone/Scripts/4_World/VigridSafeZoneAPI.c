#ifdef SERVER
/**
 *  SafeZone - the public API. THIS IS THE ENTIRE CONTRACT with the host game.
 *
 *  The addon has no opinion of its own about when peace applies - it does nothing at all until a
 *  host game says so. A host game with a lobby phase turns it on when the lobby opens and off at
 *  the instant the match actually goes live.
 *
 *  While active, exactly two things change and nothing else does:
 *
 *    - pulling the trigger on a firearm does nothing: no shot, no round consumed, no noise;
 *    - damage inflicted by another player is discarded.
 *
 *  Raising a weapon, aiming down sights, swinging melee, reloading and every user action all behave
 *  normally, and so does everything that is not another player: falls, drowning, infected, animals
 *  and any scripted damage the host game applies itself still land.
 *
 *  Every method is safe to call at any time, including before anything is initialised, so a host
 *  game never has to null-check.
 *
 *  Usage from the host game (guard every call site, so removing the addon still builds):
 *
 *      #ifdef VIGRID_SAFEZONE
 *          VigridSafeZoneAPI.SetActive(true);
 *      #endif
 */
class VigridSafeZoneAPI
{
    /**
     *  Turn the truce on or off. Idempotent, and applies to players already connected as well as to
     *  anyone who joins while it is on.
     *
     *  Not persisted - a fresh server starts with the truce off.
     */
    static void SetActive(bool active)
    {
        VigridSafeZoneState.SetActive(active);
    }

    static bool IsActive()
    {
        return VigridSafeZoneState.IsActive();
    }
}
#endif
