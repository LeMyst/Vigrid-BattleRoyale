#ifndef SERVER
/**
 *  One cached leaderboard ladder on the client.
 *
 *  The solo and group boards are fetched independently and then kept for the rest of the session.
 *  That is not just an optimisation - it is what makes the tab instant. The server rate-limits
 *  leaderboard requests per player, so re-fetching on every tab click meant a fast solo->group->solo
 *  sequence got silently refused and the list rendered empty.
 *
 *  Holding the data indefinitely is correct here rather than merely convenient: a ladder only
 *  changes when a match ends, and this server restarts its process between matches, so the
 *  connection - and this cache with it - never outlives the data it is caching.
 */
class BattleRoyaleLeaderboardBoard
{
    ref array<string> names;
    ref array<int> matches;
    ref array<int> wins;
    ref array<int> kills;
    ref array<int> points;

    //! The viewing player's own standing. rank 0 means "not on this ladder".
    int self_rank;
    int self_wins;
    int self_points;

    //! False until the server has answered at least once. Drives the "loading" state, and is what
    //! stops an empty-but-unfetched board being drawn as a genuinely empty ladder.
    bool valid;

    void BattleRoyaleLeaderboardBoard()
    {
        names = new array<string>();
        matches = new array<int>();
        wins = new array<int>();
        kills = new array<int>();
        points = new array<int>();

        Clear();
    }

    void Clear()
    {
        names.Clear();
        matches.Clear();
        wins.Clear();
        kills.Clear();
        points.Clear();

        self_rank = 0;
        self_wins = 0;
        self_points = 0;
        valid = false;
    }

    int Count()
    {
        return names.Count();
    }
}
#endif
