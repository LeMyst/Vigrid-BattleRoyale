#ifdef SERVER
/**
 *  On-disk shape of $profile:Vigrid-BattleRoyale\last_match.json.
 *
 *  ONE match, written ONCE, read back by the NEXT server process. That is the whole feature: this
 *  server restarts between matches, so the lobby a player reconnects into is the only place a
 *  summary can reach the winner as well as the dead.
 *
 *  Consequences of "written once" worth keeping in mind before changing anything here:
 *
 *    - The file is never a journal of a match in progress. A partially-written match would carry
 *      br_position values that are "groups remaining right now" rather than finishing places, so
 *      every survivor in it would read as having placed 4th.
 *    - Because the previous file still holds match N-1 at the instant of the write, the copy-aside
 *      into .bak is a free one-deep history rather than only a torn-write guard.
 *
 *  uids ARE stored here and are NEVER sent to a client. This is a server-side artefact in the
 *  operator's own profile folder, exactly like leaderboard.json - and the uid is what lets the next
 *  process work out which row belongs to the player asking. Take it out and the feature has no way
 *  to find you.
 *
 *  Note every array member needs `ref`. A missing `ref` on a JSON-deserialised array is a live bug
 *  class in this repo (see BattleRoyalePOIsData and PartiesWebhook).
 */
class BattleRoyaleLastMatchRow
{
    string uid;   //!< SteamID64. Server-side only - never leaves this file.
    string name;

    int place;       //!< finishing place, from PlayerBase.br_position at the moment of exit
    int kills;
    int damage;      //!< normalised so one full-health player == 100
    int hits;
    int survived_s;
    int group_index; //!< match-start party snapshot. Solo players each get their own index.

    //--- The death recap. Copied off the BattleRoyaleDeathRecord, so what the death screen showed
    //--- live and what the lobby card shows later can never disagree.
    int cause;               //!< BattleRoyaleKillCause. NONE for the winner.
    string killer_name;
    string weapon_type;      //!< classname; localised client-side
    int distance_m;          //!< -1 when there was no shooter to measure from
    int killer_health_pct;   //!< -1 unknown
    int damage_to_killer;

    void BattleRoyaleLastMatchRow()
    {
        //--- Explicit, because a JSON file written by an older build simply omits newer keys and the
        //--- deserialiser leaves whatever the constructor set. Zero is a valid place and a valid
        //--- kill count, so the "unknown" fields must not default to it.
        place = 0;
        kills = 0;
        damage = 0;
        hits = 0;
        survived_s = 0;
        group_index = -1;
        cause = BattleRoyaleKillCause.UNKNOWN;
        distance_m = -1;
        killer_health_pct = -1;
        damage_to_killer = 0;
    }
}

class BattleRoyaleLastMatchFile
{
    int version = BR_LASTMATCH_FILE_VERSION;
    int saved_at;      //!< BattleRoyaleTime.NowHours(), for an operator reading the file by hand
    int field_size;    //!< groups when `grouped`, players otherwise
    bool grouped;      //!< were parties actually in play - see BR_LASTMATCH_FLAG_GROUPED
    string winner_name;
    int winner_kills;
    ref array<ref BattleRoyaleLastMatchRow> rows;

    void BattleRoyaleLastMatchFile()
    {
        rows = new array<ref BattleRoyaleLastMatchRow>();
        field_size = 0;
        grouped = false;
        winner_name = "";
        winner_kills = 0;
    }
}
#endif
