#ifdef SERVER
/**
 *  Per-match statistics: damage dealt, hits landed, survival time and the party snapshot.
 *
 *  Shaped after BattleRoyaleKillLedger deliberately - uid-keyed maps, NO object references, cleared
 *  in BeginMatch. Holding a PlayerBase here would make a freed entity a failure mode, and every one
 *  of these figures has to outlive its owner's death and disconnect to be worth anything.
 *
 *  This class owns the one clock the mod never had. Nothing recorded a match start time, so survival
 *  time was not derivable at all; m_MatchStartMs is set at the exact instant deaths begin to count,
 *  which is the same instant the leaderboard opens its scoring window.
 *
 *  Kills are NOT tracked here. BattleRoyaleKillLedger already owns them, already survives a killer's
 *  death, and already amends the ladder for a posthumous kill - a second tally would be a second
 *  chance to disagree.
 */
class BattleRoyaleMatchStats
{
    protected static ref BattleRoyaleMatchStats m_Instance;

    //--- GetGame().GetTime() at BeginMatch. 0 means no match has started in this process.
    protected int m_MatchStartMs;

    //--- Has a match started at all. Distinct from m_Recording, which also goes false at match end -
    //--- the write at shutdown needs "was there ever a match", not "is one running".
    protected bool m_HasMatch;
    protected bool m_Recording;

    //--- Number of GROUPS (or players when parties are not in play) at the moment gameplay began.
    protected int m_FieldSize;

    //--- Were parties ACTUALLY in play, from VigridPartyAPI.IsReady() - never from #ifdef
    //--- VIGRID_PARTY. Party ships in this repo so the ifdef is true on every server, while the
    //--- addon can still be disabled in party_settings.json, in which case the group partition
    //--- degrades to one group per player and m_FieldSize becomes the player count. That is correct
    //--- for partitioning and wrong for any label saying "squads".
    protected bool m_Grouped;

    protected ref map<string, float> m_Damage;      //!< uid -> damage dealt, one full player == 100
    protected ref map<string, int> m_Hits;
    protected ref map<string, int> m_GroupIndex;    //!< match-start snapshot
    protected ref map<string, string> m_Names;      //!< match-start snapshot

    /**
     *  Damage between one ordered pair, keyed "attacker|victim".
     *
     *  Flat rather than a map-of-maps: one lookup instead of two, no lazily-created inner map and no
     *  owned refs to clear. `|` cannot appear in a SteamID64 and both halves are fixed 17 digits, so
     *  the key is unambiguous and 35 characters.
     *
     *  Bounded by construction: at most one entry per ordered pair that actually exchanged fire, so
     *  the theoretical ceiling for a 64-player match is a few thousand short strings and the real
     *  figure is two orders of magnitude lower. NOTHING ever iterates it - it is written per hit and
     *  read exactly once per death, by key. It is also never persisted: last_match.json keeps only
     *  the resolved damage_to_killer scalar per row.
     */
    protected ref map<string, float> m_PairDamage;

    //--- Finished rows, in exit order. The LAST one recorded is the winner.
    protected ref array<ref BattleRoyaleLastMatchRow> m_Rows;

    //--- RemovePlayer is documented to fire twice per kill. This class needs its OWN dedupe set:
    //--- BattleRoyaleLeaderboard's is protected AND additionally gated on enable_leaderboard, so a
    //--- server running with the ladder switched off would record no summary at all.
    protected ref set<string> m_Recorded;

    /**
     *  The PREVIOUS match, read from disk once at construction and never re-read.
     *
     *  This is the only thing ever served to a client. Because the file is written exactly once, at
     *  match end, this reference is immutable for the whole process lifetime - which is what makes
     *  serving it safe with no sequence number and no invalidation. NULL means no previous match.
     */
    protected ref BattleRoyaleLastMatchFile m_Previous;

    void BattleRoyaleMatchStats()
    {
        m_Damage = new map<string, float>();
        m_Hits = new map<string, int>();
        m_GroupIndex = new map<string, int>();
        m_Names = new map<string, string>();
        m_PairDamage = new map<string, float>();
        m_Rows = new array<ref BattleRoyaleLastMatchRow>();
        m_Recorded = new set<string>();

        m_MatchStartMs = 0;
        m_HasMatch = false;
        m_Recording = false;
        m_FieldSize = 0;
        m_Grouped = false;

        //--- Same shape as BattleRoyaleLeaderboard's LoadFromDisk call in its own constructor, so
        //--- there is no separate Init() call site to forget.
        m_Previous = BattleRoyaleLastMatchStore.Load();
    }

    static BattleRoyaleMatchStats GetInstance()
    {
        if (!m_Instance)
            m_Instance = new BattleRoyaleMatchStats();

        return m_Instance;
    }

    //------------------------------------------------------------------------------------------
    //--- Match lifecycle
    //------------------------------------------------------------------------------------------

    /**
     *  Open the recording window, at the exact instant deaths begin to count.
     *
     *  `group_index` and `names` are resolved by the caller in 5_Mission, where VigridPartyAPI is
     *  reachable, and handed down as plain data - the same arrangement BattleRoyaleLeaderboard.BeginMatch
     *  uses, and for the same reason: this is 4_World and has no declared dependency on Party's PBO.
     *
     *  The snapshot is exact for the whole match because VigridPartyAPI.SetFormationLocked(true)
     *  fires in 1_BattleRoyaleDebug.Deactivate(), so composition cannot change afterwards. No live
     *  roster query is ever needed, which is what lets a squad total be computed for players who are
     *  long dead.
     */
    void BeginMatch(int field_size, bool grouped, map<string, int> group_index, map<string, string> names)
    {
        m_Damage.Clear();
        m_Hits.Clear();
        m_GroupIndex.Clear();
        m_Names.Clear();
        m_PairDamage.Clear();
        m_Rows.Clear();
        m_Recorded.Clear();

        m_MatchStartMs = GetGame().GetTime();
        m_HasMatch = true;
        m_Recording = true;
        m_FieldSize = field_size;
        m_Grouped = grouped;

        if (group_index)
        {
            for (int g = 0; g < group_index.Count(); g++)
                m_GroupIndex.Set(group_index.GetKey(g), group_index.GetElement(g));
        }

        if (names)
        {
            for (int n = 0; n < names.Count(); n++)
                m_Names.Set(names.GetKey(n), names.GetElement(n));
        }

        //--- Built in steps: a single expression carrying this many terms is a hard
        //--- "Formula too complex" compile error that only surfaces when the module loads.
        string line = "[Stats] begin field=" + m_FieldSize;
        line = line + " grouped=" + m_Grouped;
        line = line + " roster=" + m_GroupIndex.Count();
        BattleRoyaleUtils.Info(line);
    }

    /**
     *  Close the recording window and write the match to disk. The one write.
     *
     *  Called from 9_BattleRoyaleRestart.Activate(). Returns immediately when no match ever started,
     *  which is what stops a server stopped in the lobby from replacing a good previous match with an
     *  empty one - largely belt-and-braces, since the state machine only moves forward and a
     *  lobby-stopped server never reaches that state at all.
     */
    void EndMatch()
    {
        m_Recording = false;

        if (!m_HasMatch)
        {
            BattleRoyaleUtils.Info("[LastMatch] No match was played in this process - leaving last_match.json alone");
            return;
        }

        BattleRoyaleLastMatchStore.Save( BuildFile() );
    }

    //! The previous match, or NULL. The only thing ever served to a client.
    BattleRoyaleLastMatchFile GetPrevious()
    {
        return m_Previous;
    }

    /**
     *  Assemble the on-disk file from this match's rows.
     *
     *  The winner is the LAST row recorded, because RemovePlayer runs in exit order and the winners
     *  are removed last, by KickWinner. With a squad win that is one of the winning group; any of
     *  them is a correct answer to "who won", and picking the last keeps this a single read rather
     *  than a placement scan.
     */
    protected BattleRoyaleLastMatchFile BuildFile()
    {
        ref BattleRoyaleLastMatchFile store = new BattleRoyaleLastMatchFile();
        int count = 0;
        int i = 0;

        store.field_size = m_FieldSize;
        store.grouped = m_Grouped;

        count = m_Rows.Count();

        //--- The winner is read BEFORE sorting, because it is the LAST exit rather than the best
        //--- place: RemovePlayer runs in exit order and KickWinner removes the winners last.
        if (count > 0)
        {
            BattleRoyaleLastMatchRow winner = m_Rows.Get(count - 1);
            store.winner_name = winner.name;
            store.winner_kills = winner.kills;
        }

        /**
         *  Sorted by PLACE, best first.
         *
         *  m_Rows is in exit order, which is death order - i.e. very nearly reverse standings. Left
         *  unsorted the table renders last place at the top, which is what shipped until a real
         *  two-player match showed the winner underneath the loser.
         *
         *  The fake fixture could never have caught it: it was written place 1..40 in file order, so
         *  it was already sorted by construction and looked perfect. A fixture that is ordered the
         *  way the output should be ordered cannot test the ordering.
         *
         *  Insertion sort: the field is capped at 64, so this is trivially small, and it is stable
         *  for equal places. Sorting BEFORE the cap also means a truncated table keeps the BEST
         *  places rather than whoever happened to die first.
         */
        ref array<ref BattleRoyaleLastMatchRow> sorted = new array<ref BattleRoyaleLastMatchRow>();
        for (i = 0; i < count; i++)
        {
            BattleRoyaleLastMatchRow row = m_Rows.Get(i);
            if (!row)
                continue;

            int insert_at = sorted.Count();
            for (int j = 0; j < sorted.Count(); j++)
            {
                if (row.place < sorted.Get(j).place)
                {
                    insert_at = j;
                    break;
                }
            }

            sorted.InsertAt(row, insert_at);
        }

        for (i = 0; i < sorted.Count(); i++)
        {
            if (i >= BR_LASTMATCH_MAX_ROWS)
                break;

            store.rows.Insert(sorted.Get(i));
        }

        return store;
    }

    bool HasMatch()
    {
        return m_HasMatch;
    }

    bool IsRecording()
    {
        return m_Recording;
    }

    int GetFieldSize()
    {
        return m_FieldSize;
    }

    bool IsGrouped()
    {
        return m_Grouped;
    }

    array<ref BattleRoyaleLastMatchRow> GetRows()
    {
        return m_Rows;
    }

    //------------------------------------------------------------------------------------------
    //--- Accumulation
    //------------------------------------------------------------------------------------------

    /**
     *  Credit one landed hit.
     *
     *  `dealt` is already normalised by the caller so that one full-health player is 100, and is
     *  already clamped to what the victim actually had left. See PlayerBase.EEHitBy for why the
     *  number is measured as a health/blood delta rather than read off TotalDamageResult.
     */
    void NoteDamage(string attacker_uid, string victim_uid, float dealt)
    {
        float running = 0;

        if (!m_Recording)
            return;
        if (attacker_uid == "")
            return;
        if (attacker_uid == victim_uid)
            return;
        if (dealt <= 0)
            return;

        running = 0;
        if (m_Damage.Contains(attacker_uid))
            running = m_Damage.Get(attacker_uid);
        m_Damage.Set(attacker_uid, running + dealt);

        int hits = 0;
        if (m_Hits.Contains(attacker_uid))
            hits = m_Hits.Get(attacker_uid);
        m_Hits.Set(attacker_uid, hits + 1);

        if (victim_uid == "")
            return;

        string pair_key = PairKey(attacker_uid, victim_uid);
        float pair_running = 0;
        if (m_PairDamage.Contains(pair_key))
            pair_running = m_PairDamage.Get(pair_key);
        m_PairDamage.Set(pair_key, pair_running + dealt);

        //--- Per hit, at Trace. Noisy by construction, and that is the point: without it "this player
        //--- dealt no damage" and "this player's hits were rejected before they got here" are the
        //--- same log line - i.e. no line at all. That ambiguity cost a whole two-client match once,
        //--- when a victim visibly finished on 74 health while their attacker was credited with zero.
        string line = "[Stats] hit " + attacker_uid;
        line = line + " -> " + victim_uid;
        line = line + " dealt=" + Math.Round(dealt);
        line = line + " total=" + GetDamage(attacker_uid);
        BattleRoyaleUtils.Trace(line);
    }

    /**
     *  Why a hit was NOT credited. Called from PlayerBase.BR_NoteDamageDealt at each of its exits.
     *
     *  The mirror of the trace above, and the more useful half: a rejected hit is otherwise
     *  completely silent, so a damage total that is wrong and a damage total that is right are
     *  indistinguishable from the log.
     */
    void NoteDamageRejected(string reason, string attacker_uid, string victim_uid)
    {
        if (!m_Recording)
            return;

        string line = "[Stats] hit REJECTED (" + reason + ")";
        line = line + " attacker=" + attacker_uid;
        line = line + " victim=" + victim_uid;
        BattleRoyaleUtils.Trace(line);
    }

    protected string PairKey(string attacker_uid, string victim_uid)
    {
        return attacker_uid + "|" + victim_uid;
    }

    int GetDamage(string uid)
    {
        if (uid == "")
            return 0;
        if (!m_Damage.Contains(uid))
            return 0;

        return Math.Round(m_Damage.Get(uid));
    }

    int GetHits(string uid)
    {
        if (uid == "")
            return 0;
        if (!m_Hits.Contains(uid))
            return 0;

        return m_Hits.Get(uid);
    }

    //! How much `attacker` dealt to `victim` across the whole match. Read once, at the victim's death.
    int GetPairDamage(string attacker_uid, string victim_uid)
    {
        string pair_key = "";

        if (attacker_uid == "")
            return 0;
        if (victim_uid == "")
            return 0;

        pair_key = PairKey(attacker_uid, victim_uid);
        if (!m_PairDamage.Contains(pair_key))
            return 0;

        return Math.Round(m_PairDamage.Get(pair_key));
    }

    //! Match-start party index, or -1 for a player who was not in the field. Solo players each hold
    //! their own index, so callers never need an "are parties on" branch.
    int GetGroupIndex(string uid)
    {
        if (uid == "")
            return -1;
        if (!m_GroupIndex.Contains(uid))
            return -1;

        return m_GroupIndex.Get(uid);
    }

    //! True when both players were on the same squad at match start. Used to refuse friendly-fire
    //! damage credit - a card crediting a player for shooting their own squadmate reads as a bug.
    bool AreTeammates(string a_uid, string b_uid)
    {
        int a_group = 0;
        int b_group = 0;

        if (a_uid == "")
            return false;
        if (b_uid == "")
            return false;
        if (!m_Grouped)
            return false;

        a_group = GetGroupIndex(a_uid);
        if (a_group < 0)
            return false;

        b_group = GetGroupIndex(b_uid);
        if (b_group < 0)
            return false;

        return a_group == b_group;
    }

    //------------------------------------------------------------------------------------------
    //--- Rows
    //------------------------------------------------------------------------------------------

    /**
     *  Book one player's finished row. Called from BattleRoyaleState.RemovePlayer, the single exit
     *  funnel - killed, disconnected, disconnected while unconscious, force-logged out, or kicked as
     *  the winner all reach it.
     *
     *  Two orderings this depends on, both already relied upon by BattleRoyaleLeaderboard.RecordExit:
     *
     *    - It runs BEFORE m_Players.RemoveItem, so player.GetBRPosition() is still the finishing
     *      place. The UpdateTopPosition that follows only rewrites the survivors.
     *    - The death record already exists, because BattleRoyaleServer.OnPlayerKilled calls
     *      RecordDeath before it calls RemovePlayer.
     *
     *  Gated on m_Recording only, NOT on enable_leaderboard: the summary is independent of the ladder.
     *
     *  `death` is handed in rather than looked up, because the table that holds it lives on
     *  BattleRoyaleSpectators in 5_Mission and this class is 4_World. The caller is 5_Mission and can
     *  reach both. NULL means this player was never eliminated - the winner.
     */
    void RecordExit(PlayerBase player, BattleRoyaleDeathRecord death)
    {
        string uid = "";
        BattleRoyaleLastMatchRow row = NULL;

        if (!m_Recording)
            return;
        if (!player)
            return;

        uid = player.player_steamid;
        if (uid == "" && player.GetIdentity())
            uid = player.GetIdentity().GetPlainId();

        if (uid == "")
            return;

        //--- RemovePlayer fires twice per kill.
        if (m_Recorded.Find(uid) >= 0)
            return;
        m_Recorded.Insert(uid);

        row = new BattleRoyaleLastMatchRow();
        row.uid = uid;
        row.name = ResolveName(player, uid);
        row.place = player.GetBRPosition();
        row.kills = BattleRoyaleKillLedger.GetInstance().GetKills(uid);
        row.damage = GetDamage(uid);
        row.hits = GetHits(uid);
        row.group_index = GetGroupIndex(uid);
        row.survived_s = SurvivalSeconds(death);

        if (death)
        {
            row.cause = death.cause;
            row.killer_name = death.killer_name;
            row.weapon_type = death.weapon_type;
            row.distance_m = death.distance_m;
            row.killer_health_pct = death.killer_health_pct;
            row.damage_to_killer = death.damage_to_killer;
        }
        else
        {
            //--- No death record means they were never eliminated: the winner, kicked by KickWinner.
            //--- Without this their card renders a blank recap panel, which reads as broken, and it
            //--- is the row everybody looks at first.
            row.cause = BattleRoyaleKillCause.NONE;
        }

        m_Rows.Insert(row);

        string line = "[Stats] exit " + uid;
        line = line + " place=" + row.place;
        line = line + " kills=" + row.kills;
        line = line + " dmg=" + row.damage;
        line = line + " alive=" + row.survived_s + "s";
        BattleRoyaleUtils.Info(line);
    }

    /**
     *  Seconds this player lasted.
     *
     *  Taken from the death record when there is one, so it is the moment they actually fell rather
     *  than the moment the roster was tidied up. The winner has no death record and falls through to
     *  "now", which means their figure includes the ~15 s they spend on the win screen before
     *  KickWinner runs. That is accepted rather than special-cased: correcting it would make survival
     *  time disagree with the placement the leaderboard books from the very same call, and 15 s on a
     *  twenty-minute match is noise.
     */
    protected int SurvivalSeconds(BattleRoyaleDeathRecord death)
    {
        int end_ms = 0;

        if (m_MatchStartMs <= 0)
            return 0;

        end_ms = GetGame().GetTime();

        if (death)
            end_ms = death.death_ms;

        if (end_ms <= m_MatchStartMs)
            return 0;

        return (end_ms - m_MatchStartMs) / 1000;
    }

    //! Snapshot name first: it was taken while everyone still had an identity, which a disconnecting
    //! player no longer does.
    protected string ResolveName(PlayerBase player, string uid)
    {
        string name = "";

        if (m_Names.Contains(uid))
            name = m_Names.Get(uid);
        if (name != "")
            return name;

        name = BattleRoyaleKillAttribution.NameOfPlayer(player);
        if (name != "")
            return name;

        return uid;
    }
}
#endif
