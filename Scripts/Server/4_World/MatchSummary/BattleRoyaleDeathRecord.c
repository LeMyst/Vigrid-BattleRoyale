#ifdef SERVER
/**
 *  One death. Written once, read for the rest of the match.
 *
 *  Lives in 4_World rather than beside BattleRoyaleSpectators, which owns and writes it, because it
 *  now has a second consumer: BattleRoyaleMatchStats.RecordExit copies the recap onto the summary
 *  row, and that class has to be 4_World so PlayerBase.EEHitBy can reach it. A 4_World method cannot
 *  name a 5_Mission type, so the shared plain-data record moves down to the lower stage and the
 *  5_Mission owner keeps holding it.
 *
 *  Plain strings and ints only. BattleRoyaleSpectators holds no object reference at all - a freed
 *  PlayerBase is never a failure mode there - and every field added here has to keep that true.
 */
class BattleRoyaleDeathRecord
{
    string victim_uid;   //!< SteamID64. Map key.
    string victim_name;  //!< cached at death, so rendering a name never needs an identity
    string killer_uid;   //!< SteamID64 of the responsible PLAYER, or "" for every non-player cause
    int death_ms;        //!< GetGame().GetTime() at death
    string party_id;     //!< VigridPartyAPI.GetPartyId() snapshot, "" when solo or no party addon
    vector death_pos;    //!< where they fell. Used to pick the NEAREST living player as a fallback.

    //--- The death recap, resolved once by BattleRoyaleKillAttribution.ResolveKillDetails at the
    //--- moment of death, while the weapon or device that did it is still in hand. Assigned as plain
    //--- field writes right after construction rather than through the constructor: it is already at
    //--- six arguments, and twelve would be unreadable at both call sites.
    int cause;               //!< BattleRoyaleKillCause
    string killer_name;
    string weapon_type;      //!< classname; localised client-side
    int distance_m;          //!< -1 when there was no shooter to measure from
    int killer_health_pct;   //!< 0-100, -1 unknown
    int damage_to_killer;    //!< what the victim had already dealt to whoever finished them

    void BattleRoyaleDeathRecord(string victim, string name, string killer, int time_ms, string party, vector position)
    {
        victim_uid = victim;
        victim_name = name;
        killer_uid = killer;
        death_ms = time_ms;
        party_id = party;
        death_pos = position;

        //--- 0 is a legitimate distance and a legitimate health reading, so "unknown" must not be 0.
        cause = BattleRoyaleKillCause.UNKNOWN;
        killer_name = "";
        weapon_type = "";
        distance_m = -1;
        killer_health_pct = -1;
        damage_to_killer = 0;
    }
}
#endif
