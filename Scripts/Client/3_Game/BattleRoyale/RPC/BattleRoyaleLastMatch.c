#ifndef SERVER
/**
 *  The previous match, cached on the client.
 *
 *  Its own class rather than a reuse of BattleRoyaleLeaderboardBoard: that one's arrays are named
 *  matches/wins/points, and holding places, survival times and damage in them would read fine for
 *  about a week.
 *
 *  Fetched once per session and then kept, exactly like a ladder, and for a stronger reason - the
 *  file behind it is written once per server process, so it physically cannot change while this
 *  connection is open.
 *
 *  NO SteamID64s. The uid exists only in the server's own last_match.json; self_index is what tells
 *  this client which row is theirs.
 */
class BattleRoyaleLastMatch
{
    ref array<string> names;
    ref array<int> places;
    ref array<int> kills;
    ref array<int> damage;
    ref array<int> survived;   //!< seconds
    ref array<int> groups;     //!< match-start squad index; solo players each have their own

    /**
     *  Which row is the viewing player's, or -1.
     *
     *  -1 is the COMMON case in a lobby, not an edge case: most people there connected after the
     *  server restarted and so never played the match being shown. The card must be hidden outright
     *  for them, never rendered zeroed - a zeroed card claims they placed #0 with no kills.
     */
    int self_index;

    //! Groups when `grouped`, players otherwise.
    int field_size;

    //! BR_LASTMATCH_FLAG_GROUPED / _TRUNCATED.
    int flags;

    //! False until the server has answered at least once. Distinguishes "still loading" from
    //! "there genuinely was no previous match", which look identical on screen.
    bool valid;

    void BattleRoyaleLastMatch()
    {
        names = new array<string>();
        places = new array<int>();
        kills = new array<int>();
        damage = new array<int>();
        survived = new array<int>();
        groups = new array<int>();

        Clear();
    }

    void Clear()
    {
        names.Clear();
        places.Clear();
        kills.Clear();
        damage.Clear();
        survived.Clear();
        groups.Clear();

        self_index = -1;
        field_size = 0;
        flags = 0;
        valid = false;
    }

    int Count()
    {
        return names.Count();
    }

    //! Were parties actually in play. Everything that says "squad" has to ask this first: with the
    //! party manager disabled, field_size is just the player count wearing a squad label.
    bool IsGrouped()
    {
        return (flags & BR_LASTMATCH_FLAG_GROUPED) != 0;
    }

    /**
     *  Did the row cap bite.
     *
     *  The squad totals are summed client-side from this table, so a squadmate missing from it
     *  produces a smaller, WRONG total with nothing to signal it. When this is true the squad block
     *  is hidden instead - no figure beats a wrong figure, the same rule as BR_HUD_GROUPS_NONE.
     */
    bool IsTruncated()
    {
        return (flags & BR_LASTMATCH_FLAG_TRUNCATED) != 0;
    }
}
#endif
