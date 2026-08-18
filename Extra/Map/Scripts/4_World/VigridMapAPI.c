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

    //--- Hot zones: static circles the host marks as regions of interest. Purely decoration as far
    //--- as this addon is concerned - it draws them and knows nothing else about them. Held as a
    //--- copy rather than the caller's arrays, which are owned by the host and may be cleared under
    //--- us between frames.
    private static ref array<vector> s_HotZoneCenters = new array<vector>();
    private static ref array<float> s_HotZoneRadii = new array<float>();
    private static int s_HotZoneSeq;

    /**
     *  Publish the hot zones to draw.
     *
     *  Safe to call every frame, like SetZones: the incoming pair is compared against the stored copy
     *  and s_HotZoneSeq only moves when something really changed. Passing empty or NULL arrays clears
     *  them. A radius of 0 or a centre of "0 0 0" is skipped at draw time rather than rejected here,
     *  so the host's indices and this addon's stay aligned.
     */
    static void SetHotZones(array<vector> centers, array<float> radii)
    {
        int count = 0;
        if (centers && radii)
        {
            count = centers.Count();
            if (radii.Count() < count)
                count = radii.Count();
        }

        bool changed = false;

        if (s_HotZoneCenters.Count() != count)
            changed = true;

        //--- Only worth walking when the counts already agree; a length change is decisive on its own.
        if (!changed)
        {
            for (int c = 0; c < count; c++)
            {
                //--- One array read per line. A read sharing an expression with a call has been
                //--- measured in this codebase to return another array's contents entirely.
                vector incoming_center = centers[c];
                vector stored_center = s_HotZoneCenters[c];
                if (incoming_center != stored_center)
                {
                    changed = true;
                    break;
                }

                float incoming_radius = radii[c];
                float stored_radius = s_HotZoneRadii[c];
                if (incoming_radius != stored_radius)
                {
                    changed = true;
                    break;
                }
            }
        }

        if (!changed)
            return;

        s_HotZoneCenters.Clear();
        s_HotZoneRadii.Clear();

        for (int i = 0; i < count; i++)
        {
            vector center = centers[i];
            float radius = radii[i];
            s_HotZoneCenters.Insert(center);
            s_HotZoneRadii.Insert(radius);
        }

        s_HotZoneSeq = s_HotZoneSeq + 1;

        VigridMapLog.Debug("SetHotZones " + s_HotZoneCenters.Count() + " zone(s), seq " + s_HotZoneSeq);
    }

    //! Equivalent to SetHotZones with nothing in it. Call on teardown, for the same reason
    //! ClearZones exists: this state is static and outlives the object that pushed it.
    static void ClearHotZones()
    {
        SetHotZones(NULL, NULL);
    }

    static int GetHotZoneCount()
    {
        return s_HotZoneCenters.Count();
    }

    static vector GetHotZoneCenter(int index)
    {
        if (index < 0 || index >= s_HotZoneCenters.Count())
            return vector.Zero;

        return s_HotZoneCenters[index];
    }

    static float GetHotZoneRadius(int index)
    {
        if (index < 0 || index >= s_HotZoneRadii.Count())
            return 0;

        return s_HotZoneRadii[index];
    }

    static int GetHotZoneSeq()
    {
        return s_HotZoneSeq;
    }

    //--- Where to draw "you", when that is not where the local player's body is. Zero means "no
    //--- override in force", which is the normal case and costs one vector compare per read.
    private static vector s_SelfOverride;

    /**
     *  Publish a position to draw the local player at, instead of their body.
     *
     *  ⚠ STATED AS A GENERAL CONTRACT, NOT AS ITS CALLER'S USE CASE, exactly like
     *  VigridPartyAPI.SetMemberHidden: A HOST MOD CAN PUT THE LOCAL PLAYER SOMEWHERE THAT IS NOT
     *  WHERE THEIR BODY IS. This addon cannot detect that and must not try - it has no concept of a
     *  match, a camera or a spectator - so the host asserts it.
     *
     *  What prompted it: a spectating admin's body is parked somewhere as a network anchor while the
     *  camera flies, so GetGame().GetPlayer().GetPosition() answers the anchor and the fullscreen
     *  map drew the self glyph on it. The minimap was always right, because it re-centres on the
     *  camera every tick and never asks the player at all.
     *
     *  ONLY THE POSITION IS OVERRIDDEN, never the heading. The heading is already read from the
     *  camera on both maps and is therefore already correct while spectating - see the docblock on
     *  VigridMapMinimap.DrawHeadingArrow for why it can never come from body yaw.
     *
     *  Safe to call every frame; the caller owns the clear. Idempotent and session-scoped.
     */
    static void SetSelfPositionOverride(vector pos)
    {
        s_SelfOverride = pos;
    }

    //! Drop the override and go back to drawing the player's own body.
    //!
    //! ⚠ THE CALLER OWNS THIS, and leaving it set strands the self glyph at a position the player
    //! left long ago - with no error and nothing on screen to say why. Clear it on every path out,
    //! including the ones that are not a clean exit. Same warning, and the same reason, as
    //! VigridPartyAPI.SetMemberHidden's.
    static void ClearSelfPositionOverride()
    {
        s_SelfOverride = vector.Zero;
    }

    static bool HasSelfPositionOverride()
    {
        return s_SelfOverride != vector.Zero;
    }

    static vector GetSelfPositionOverride()
    {
        return s_SelfOverride;
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
