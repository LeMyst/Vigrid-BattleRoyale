/**
 *  The per-player table behind the admin console's player operations and its live scoreboard.
 *
 *  UNGUARDED, and serialized as JSON, for the reasons set out in BattleRoyaleAdminStatus.c - the
 *  ordering hazard of sequential rpc.Write/ctx.Read pairs is worse here than there, because this
 *  payload has fourteen fields per row.
 *
 *  ⚠️ THIS ONE CARRIES SteamID64s, and that is why it is answered PER IDENTITY to an authorized
 *  admin and never broadcast. The precedent is SetAdminPlayerList, which carries uids for exactly
 *  this reason, against SetLeaderboard, which deliberately omits them because it goes to everybody.
 *  A uid is unavoidable here: every player operation has to name its subject, and a display name is
 *  not a key - `Survivor` and `Survivor (2)` are the ordinary case with Steam name lookup off.
 *
 *  One table serves BOTH #301 and #304 rather than two overlapping payloads. Which columns are
 *  meaningful depends on the phase, and neither consumer has to care: in the lobby `ready` and
 *  `loaded` carry the answer and the stats are zero; in a match it is the other way round. Sending
 *  one table means the two views can never disagree about who is in the match.
 */
class BattleRoyaleAdminRosterRow
{
    string uid = "";
    string name = "";

    //--- Match state ------------------------------------------------------------------------------
    bool   alive = false;       //!< holds a live body AND is on the current state's roster
    bool   in_state = false;    //!< on the roster at all - false for an admin or a late joiner
    bool   spectating = false;
    int    group = BR_ADMIN_GROUPS_UNKNOWN;  //!< match-local party index, or the sentinel when unknown

    //--- Lobby ------------------------------------------------------------------------------------
    bool   ready = false;
    bool   loaded = false;      //!< their client has reported PlayerLoadedIn

    //--- Scoreboard (#304). Zero until a match is recording.
    int    kills = 0;
    int    damage = 0;          //!< "fraction of a player removed", so one full-health kill is 100
    int    hits = 0;
    int    place = 0;           //!< 0 = still playing
    int    survived_s = 0;

    //--- Late-join kick. -1 when no kick is pending, which is the overwhelmingly common case.
    //--- Surfaced because there was previously NO way to see who was on a countdown, let alone
    //--- cancel one.
    int    late_join_seconds = -1;
}

class BattleRoyaleAdminRoster
{
    //--- Deliberately not a bool: a client that never got a reply and a server with an empty roster
    //--- must be distinguishable, or an empty table reads as "everyone left".
    bool   valid = false;

    //--- True when BattleRoyaleMatchStats is recording, i.e. when the scoreboard columns mean
    //--- anything. The panel hides them rather than painting a column of zeroes that look real.
    bool   scoring = false;

    //--- True when the reply was cut at BR_ADMIN_ROSTER_MAX_ROWS. The panel says so rather than
    //--- quietly showing a partial population - same reasoning as the last-match table's TRUNCATED
    //--- flag, where a silently short table makes every derived total wrong.
    bool   truncated = false;

    ref array<ref BattleRoyaleAdminRosterRow> rows = new array<ref BattleRoyaleAdminRosterRow>();
}
