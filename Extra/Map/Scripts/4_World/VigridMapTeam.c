/**
 *  Vigrid Map - the ONLY place this addon knows the Party addon exists.
 *
 *  Every method body here is a #ifdef VIGRID_PARTY / #else pair, and nothing else under Extra/Map/
 *  carries that guard. That is the whole point of the file: whether Party is installed becomes one
 *  question asked in one place, so the rest of the addon reads as if teammates simply exist or
 *  simply do not.
 *
 *  With party.pbo absent, every method returns an empty answer - IsAvailable() is false, the counts
 *  are zero - and the renderers draw nothing extra. `Party/config.cpp` renamed to `config.cpp.disabled`
 *  must still produce a working map; that is the acceptance test this file exists to pass.
 *
 *  4_World, beside VigridMapAPI and for the same reason: VigridPartyAPI is 4_World too, and a class
 *  in one addon can call a class in another at the same stage - the marker store already does. Being
 *  here rather than in 5_Mission is also what lets the SERVER half exist, which is what keeps the
 *  marker store's own Party calls out of the addon's guard budget.
 *
 *  SPELLING. Party writes `Colour`, this addon writes `Color`. This file is the seam, so it takes
 *  the Map spelling and does the translation. Do not "fix" either side to match the other.
 */
class VigridMapTeam
{
#ifndef SERVER

    /**
     *  The single gate. False means: Party is not built in, or it is switched off, or the local
     *  player is not in a party. A caller that checks this once may then loop over GetCount()
     *  without re-checking anything.
     */
    static bool IsAvailable()
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.HasParty();
#else
        return false;
#endif
    }

    //! Whether member positions are old enough to be worth dimming. Never a reason to hide.
    static bool IsStale()
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.IsStateStale();
#else
        return false;
#endif
    }

    /**
     *  Party COMPOSITION sequence - it moves when somebody joins or leaves, and never when somebody
     *  walks. Useful for dropping cached per-member state; useless as a repaint trigger.
     */
    static int GetSeq()
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetRosterSeq();
#else
        return -1;
#endif
    }

    static int GetCount()
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetMemberCount();
#else
        return 0;
#endif
    }

    static int GetSelfIndex()
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetSelfIndex();
#else
        return -1;
#endif
    }

    static bool IsSlotVisible(int index)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.IsMemberVisible(index);
#else
        return false;
#endif
    }

    /**
     *  World position of a teammate, or vector.Zero when there is no usable data.
     *
     *  Do NOT ask this for your own slot. The local player is absent from the client entity list, so
     *  their own slot resolves to the interpolated server push and trails them by up to an interval.
     *  Draw yourself from GetGame().GetPlayer() - which also keeps the "you are here" glyph working
     *  when Party is not installed at all.
     */
    static vector GetSlotPos(int index)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetMemberPosition(index);
#else
        return vector.Zero;
#endif
    }

    /**
     *  Deliberately has no consumer yet: a CanvasWidget can draw lines and nothing else, so there is
     *  no text on the map. Kept because a tooltip or a list view is the obvious next thing to want,
     *  and adding it later means reopening the seam.
     */
    static string GetSlotName(int index)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetMemberName(index);
#else
        return "";
#endif
    }

    //! Colour of a member of the CURRENT roster, opacity baked in (CanvasWidget has no alpha).
    static int GetSlotColor(int index, float alpha)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetMemberColour(index, alpha);
#else
        return VIGRID_MAP_COLOR_TEAM_MARKER;
#endif
    }

    /**
     *  Colour for a slot number recorded somewhere else, at some other time - a placed marker stores
     *  the placer's slot server-side, and that player may since have disconnected or may never have
     *  been on this client's roster at all. Answers identically on every client, for ever, which
     *  GetSlotColor cannot promise.
     */
    static int GetColorForSlot(int slot, float alpha)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetColourForSlot(slot, alpha);
#else
        return VIGRID_MAP_COLOR_TEAM_MARKER;
#endif
    }

    //--- Party's own world pings, read-only. Drawing them here is what finally gives them a map;
    //--- this addon never writes to the ping system. Expired entries are filtered out by the Party
    //--- API, so an index here always names a live ping.

    static int GetPingCount()
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetPingCount();
#else
        return 0;
#endif
    }

    static vector GetPingPos(int index)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetPingPos(index);
#else
        return vector.Zero;
#endif
    }

    static int GetPingColor(int index, float alpha)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetPingColour(index, alpha);
#else
        return VIGRID_MAP_COLOR_TEAM_MARKER;
#endif
    }
#endif

#ifdef SERVER

    //! Stable 0-based slot in the party, -1 when solo. Recorded on a marker so it can be coloured.
    static int GetMemberSlot(PlayerBase player)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetMemberIndex(player);
#else
        return -1;
#endif
    }

    //! Party members of `player` present in `population`, excluding `player`. Empty, never null, so
    //! the caller can foreach unconditionally whether or not Party is installed.
    static array<PlayerBase> GetTeammates(PlayerBase player, array<PlayerBase> population)
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetTeammates(player, population);
#else
        return new array<PlayerBase>();
#endif
    }
#endif
}
