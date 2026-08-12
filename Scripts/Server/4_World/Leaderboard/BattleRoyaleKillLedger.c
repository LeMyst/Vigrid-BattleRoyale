#ifdef SERVER
/**
 *  The authoritative kill tally for the current match, keyed by SteamID64.
 *
 *  PlayerBase.br_kills used to be the only record, which made it impossible to credit a kill whose
 *  owner had already left: there was no object left to increment. A grenade or an armed trap
 *  routinely outlives its owner, so the tally has to outlive the object too - hence a uid map that
 *  holds no references at all, the same discipline BattleRoyaleSpectators follows.
 *
 *  br_kills is now a MIRROR of this, kept up to date for the HUD counter and the admin spectator
 *  overlay tags. This map is what the leaderboard scores.
 */
class BattleRoyaleKillLedger
{
    static ref BattleRoyaleKillLedger m_Instance;

    protected ref map<string, int> m_Kills;

    void BattleRoyaleKillLedger()
    {
        m_Kills = new map<string, int>();
    }

    static BattleRoyaleKillLedger GetInstance()
    {
        if (!m_Instance)
            m_Instance = new BattleRoyaleKillLedger();

        return m_Instance;
    }

    //! Called from BattleRoyaleLeaderboard.BeginMatch, the single "gameplay has begun" point.
    void BeginMatch()
    {
        m_Kills.Clear();
    }

    int GetKills(string uid)
    {
        if (uid == "")
            return 0;
        if (!m_Kills.Contains(uid))
            return 0;

        return m_Kills.Get(uid);
    }

    /**
     *  Credit one kill to `killer_uid` and return their new total.
     *
     *  Safe for a uid with no player object behind it any more - that is the posthumous case this
     *  exists for. The mirror and the RPC are simply skipped, and the leaderboard still scores it.
     */
    int CreditKill(string killer_uid)
    {
        PlayerBase killer = NULL;
        int total = 0;

        if (killer_uid == "")
            return 0;

        total = GetKills(killer_uid) + 1;
        m_Kills.Set(killer_uid, total);

        //--- A CORPSE resolves here too: the mod never deletes a body, so a killer who is dead but
        //--- still connected watches their own counter tick from the spectator camera.
        killer = BattleRoyaleKillAttribution.FindPlayerByUid(killer_uid);
        if (killer)
        {
            killer.br_kills = total;

            if (killer.GetIdentity())
                GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "AddPlayerKill", new Param1<int>(total), true, killer.GetIdentity(), killer);
        }

        //--- Amend rather than add: the killer may already have been recorded on their own way out of
        //--- the match, which is exactly what happens when their grenade lands after they died.
        BattleRoyaleLeaderboard.GetInstance().AmendKills(killer_uid, total);

        BattleRoyaleUtils.Info(string.Format("[Kills] %1 credited a kill, now %2", killer_uid, total));

        return total;
    }
}
#endif
