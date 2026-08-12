#ifdef SERVER
/**
 *  On-disk shape of $profile:Vigrid-BattleRoyale\leaderboard.json.
 *
 *  One row per player, carrying BOTH ladders. Solo and group results never mix - a 4-stack win is
 *  not comparable to a solo win - but they belong to the same person, so splitting them across two
 *  files would just mean two lookups and two chances to disagree.
 *
 *  Which half a result lands in is decided by the player's own group size at match start: 1 goes to
 *  solo_*, 2+ goes to group_*.
 *
 *  Note every array member needs `ref`. A missing `ref` on a JSON-deserialised array is a live bug
 *  class in this repo (see BattleRoyalePOIsData and PartiesWebhook).
 */
class BattleRoyaleLeaderboardEntry
{
    string uid;   //!< SteamID64, from PlayerIdentity.GetPlainId(). Never GetPlayerId().
    string name;  //!< last known display name, refreshed on every recorded match

    int   solo_matches;
    int   solo_wins;
    int   solo_kills;
    float solo_points;

    int   group_matches;
    int   group_wins;
    int   group_kills;
    float group_points;

    int last_seen_hours;  //!< BattleRoyaleTime.NowHours(), drives TTL pruning
}

class BattleRoyaleLeaderboardFile
{
    int version = 1;
    //--- The season this file holds. When it no longer matches leaderboard_settings.json the store
    //--- archives this file and starts a fresh ladder.
    int season = 1;
    int saved_at;
    ref array<ref BattleRoyaleLeaderboardEntry> entries;

    void BattleRoyaleLeaderboardFile()
    {
        entries = new array<ref BattleRoyaleLeaderboardEntry>();
    }
}
#endif
