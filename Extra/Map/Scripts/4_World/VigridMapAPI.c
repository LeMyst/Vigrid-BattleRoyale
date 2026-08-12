/**
 *  Vigrid Map - the entire contract with the host mod.
 *
 *  THIS IS THE ONLY SURFACE ANYTHING OUTSIDE Extra/Map/ MAY TOUCH. The addon may not reference a
 *  BattleRoyale* symbol, so the flow is push, not pull: the Battle Royale mod hands its zone
 *  geometry in, rather than this addon reaching into BattleRoyaleClient to fetch it. Every call
 *  site over there is wrapped in #ifdef VIGRID_MAP.
 *
 *  One class with per-side blocks rather than two class names. The class IS the contract, so
 *  splitting it into VigridMapAPI plus VigridMapServerAPI would double the name a caller has to
 *  remember and let the halves drift apart. Neither side ends up with an empty body.
 *
 *  Lives in 4_World, which is what makes it reachable from both directions: the host mod calls it
 *  from its 5_Mission client, and this addon's own 5_Mission renderers read it back. Later stages
 *  compile after earlier ones, so both work.
 *
 *  Every method is safe to call before anything is initialised, and safe to call every frame. No
 *  caller ever has to null-check - the same promise KillFeedAPI and VigridPartyAPI make.
 */
class VigridMapAPI
{
#ifndef SERVER
    //--- The two circles the map draws. Held here rather than in a 3_Game data bag because 4_World
    //--- is already the lowest stage both the writer and the readers can see.
    private static vector s_CurrentCenter;
    private static float s_CurrentRadius;
    private static vector s_NextCenter;
    private static float s_NextRadius;

    //--- Bumped only when a value actually moves, so a renderer can repaint on the edge instead of
    //--- diffing four fields itself. This is what makes calling SetZones every frame free.
    private static int s_ZoneSeq;

    /**
     *  Publish the current and next play areas.
     *
     *  Safe to call every frame: the values are diffed here and the sequence number only moves when
     *  something really changed. A radius of 0, or a centre of "0 0 0", clears that circle - the
     *  two are independent, since a match can have a current circle and no next one, and at the end
     *  of a match it has neither.
     */
    static void SetZones(vector cur_center, float cur_radius, vector next_center, float next_radius)
    {
        bool changed = false;

        if (s_CurrentCenter != cur_center)
            changed = true;
        if (s_CurrentRadius != cur_radius)
            changed = true;
        if (s_NextCenter != next_center)
            changed = true;
        if (s_NextRadius != next_radius)
            changed = true;

        if (!changed)
            return;

        s_CurrentCenter = cur_center;
        s_CurrentRadius = cur_radius;
        s_NextCenter = next_center;
        s_NextRadius = next_radius;
        s_ZoneSeq = s_ZoneSeq + 1;

        VigridMapLog.Debug("SetZones current=" + s_CurrentCenter + "/" + s_CurrentRadius + " next=" + s_NextCenter + "/" + s_NextRadius);
    }

    //! Equivalent to SetZones with everything zeroed. Call on teardown so a server change does not
    //! leave the previous match's circles drawn on the next one's map.
    static void ClearZones()
    {
        SetZones(vector.Zero, 0, vector.Zero, 0);
    }

    //! True once the addon is loaded. The host does not need this to call SetZones safely; it
    //! exists so a host can hide a duplicate UI of its own when this addon is present.
    static bool IsReady()
    {
        return true;
    }

    static bool HasCurrentZone()
    {
        if (s_CurrentRadius <= 0)
            return false;

        return s_CurrentCenter != vector.Zero;
    }

    static bool HasNextZone()
    {
        if (s_NextRadius <= 0)
            return false;

        return s_NextCenter != vector.Zero;
    }

    static vector GetCurrentCenter()
    {
        return s_CurrentCenter;
    }

    static float GetCurrentRadius()
    {
        return s_CurrentRadius;
    }

    static vector GetNextCenter()
    {
        return s_NextCenter;
    }

    static float GetNextRadius()
    {
        return s_NextRadius;
    }

    static int GetZoneSeq()
    {
        return s_ZoneSeq;
    }
#endif

#ifdef SERVER
    //! Drop every marker and tell every client to do the same. For a match resetting inside one
    //! server process; after a real process restart the store is empty anyway.
    static void ClearAllMarkers()
    {
        VigridMapMarkerStore.GetInstance().ClearAll();
    }

    //! Enable or disable placement at runtime, on top of the `enabled` setting. Not persisted -
    //! this is a match-state switch, not an admin preference.
    static void SetMarkersActive(bool active)
    {
        VigridMapMarkerStore.GetInstance().SetActive(active);
    }
#endif
}
