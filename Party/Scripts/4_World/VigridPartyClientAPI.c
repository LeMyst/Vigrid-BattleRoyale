#ifndef SERVER
/**
 *  Vigrid Party - the CLIENT half of the public API.
 *
 *  VigridPartyAPI is `#ifdef SERVER` from its first line, so the host game has had no supported way
 *  to say anything to the party addon from a client. This is that way, and it is deliberately the
 *  same shape: a small, neutral contract that names nothing above this addon.
 *
 *  THE VIEWPOINT. Everything the party UI shows in metres - the HUD panel's per-row distance, and
 *  the nametags' distance text, alpha fade, sort order and max-distance culling - is measured from
 *  ONE origin. That origin is normally the local player, and this overrides it.
 *
 *  It exists because the local player is not always the right answer, and the failure is silent
 *  when it is not. A host game can leave a client watching the world through a camera while
 *  GetGame().GetPlayer() still returns a body that is no longer where the client is looking - at
 *  which point every distance on screen is measured from a stale point, and nametags get culled or
 *  faded on the strength of it. The panel simply reads wrong; the nametags disappear.
 *
 *  Deliberately takes a POSITION rather than resolving one from the uid. The thing being watched
 *  need not be in the party at all, in which case there is no roster entry to look a position up
 *  from. The uid is passed alongside purely so the UI can recognise that member's own row or tag and
 *  leave its distance blank - measuring somebody's distance from themselves reads as "0m", which
 *  looks like a bug even though it is arithmetically correct.
 *
 *  Usage from the host game - guard every call site, so removing party.pbo still builds:
 *
 *      #ifdef VIGRID_PARTY
 *          VigridPartyClientAPI.SetHudViewpoint(watched_position, watched_uid);
 *      #endif
 *
 *  Purely local: nothing here is sent over the wire, and nothing here is read by the server.
 */
class VigridPartyClientAPI
{
    private static bool s_HasViewpoint = false;
    private static vector s_ViewpointPos = "0 0 0";
    private static string s_ViewpointUid = "";

    //! Hide every party HUD layer - the roster panel and the world name tags - without changing any
    //! party state. See SetHudSuppressed.
    private static bool s_HudSuppressed = false;

    /**
     *  Hide the party HUD entirely while some other overlay owns the screen.
     *
     *  Exists for a host game whose own spectator UI draws its own name tags: without this the
     *  player gets two labels stacked over the same character, one from each system, plus a party
     *  roster panel describing a party they are not currently playing in.
     *
     *  Deliberately a switch that the HOST flips rather than anything Party works out for itself -
     *  Party must not know what a Battle Royale admin is, and this way it does not have to. It is
     *  presentation only: the roster, the state feed and the pings all keep running underneath, so
     *  un-suppressing shows a current HUD rather than a stale one.
     *
     *  Safe to call every frame with unchanged values.
     */
    static void SetHudSuppressed(bool suppressed)
    {
        s_HudSuppressed = suppressed;
    }

    static bool IsHudSuppressed()
    {
        return s_HudSuppressed;
    }

    /**
     *  Measure every distance in the party UI from `position` instead of from the local player.
     *
     *  `uid` is whoever is at that position, as a PlayerIdentity.GetPlainId() - the same identity
     *  scheme as the roster, so it can be compared against roster_uids directly. Pass "" when the
     *  thing being watched is not a party member; the only consequence is that no row or tag has
     *  its distance suppressed, which is correct, because none of them is the viewpoint.
     *
     *  Safe to call every frame with unchanged values.
     */
    static void SetHudViewpoint(vector position, string uid)
    {
        s_HasViewpoint = true;
        s_ViewpointPos = position;
        s_ViewpointUid = uid;
    }

    //! Back to measuring from the local player. Safe to call when no viewpoint was ever set.
    static void ClearHudViewpoint()
    {
        s_HasViewpoint = false;
        s_ViewpointPos = "0 0 0";
        s_ViewpointUid = "";
    }

    static bool HasHudViewpoint()
    {
        return s_HasViewpoint;
    }

    static vector GetHudViewpointPos()
    {
        return s_ViewpointPos;
    }

    //! "" when the viewpoint is not a party member, so no row matches and none is suppressed.
    static string GetHudViewpointUid()
    {
        return s_ViewpointUid;
    }
}
#endif
