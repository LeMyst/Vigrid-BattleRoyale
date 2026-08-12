#ifdef SERVER
/**
 *  Battle Royale - the live leaderboard.
 *
 *  Owns the in-memory ladder, records one result per player per match, debounces writes to disk and
 *  serves the client menu. Nothing here survives the process; the JSON is the only durable state,
 *  which is why the constructor loads it.
 *
 *  ---------------------------------------------------------------------------------------------
 *  WHY RECORDING HANGS OFF RemovePlayer
 *
 *  Players leave a match by a surprising number of routes: killed in a round, disconnecting alive,
 *  disconnecting while unconscious, force-logged out by the tick, or kicked as the winner. The
 *  existing ScoreWebhook calls are scattered across three of those and miss at least one outright -
 *  a winner who quits during the 15s win screen is rejected by the
 *  `i_CurrentStateIndex > 2 && < Count()-2` guard in BattleRoyaleServer.
 *
 *  BattleRoyaleState.RemovePlayer() is the one function every route passes through, and all three
 *  subclass overrides chain to it. Crucially RemoveAllPlayers() does NOT - it clears the array
 *  directly - so the state-migration path, which must never score, is excluded for free.
 *
 *  This also means the leaderboard deliberately does not reuse that state-index window, so it is
 *  immune to the window's enable_spawn_selection_menu sensitivity and to the unguarded
 *  unconscious-disconnect path. Instead it gates on its own m_Ranking latch, opened the instant
 *  gameplay actually begins.
 *
 *  Two consequences to keep in mind when touching RecordExit:
 *    - RemovePlayer fires TWICE for a kill (once from the state, once from BattleRoyaleServer), so
 *      the uid dedupe is load-bearing, not defensive.
 *    - PlayerIdentity is already NULL on a disconnect, so uid and name come from the cached
 *      PlayerBase fields.
 */
class BattleRoyaleLeaderboard
{
    static ref BattleRoyaleLeaderboard m_Instance;

    //--- The table owns the entries; every other array of entries in this file is a weak view.
    protected ref array<ref BattleRoyaleLeaderboardEntry> m_Entries;
    protected ref map<string, BattleRoyaleLeaderboardEntry> m_ByUid;

    protected BattleRoyaleLeaderboardData m_Settings;

    //--- Per-match state, all reset by BeginMatch.
    protected ref set<string> m_Recorded;
    protected ref map<string, int> m_GroupSizes;
    protected int m_FieldSize;
    protected bool m_Ranking;

    //--- Write debounce.
    protected bool m_Dirty;
    protected int m_FlushDueMs;

    //--- Per-player RPC cooldown, keyed by SteamID64.
    protected ref map<string, int> m_RequestCooldowns;

    //--- Cached sorted views, one per ladder. Rebuilt lazily and only when the table changed, which
    //--- in practice means once per match rather than once per request.
    protected ref array<BattleRoyaleLeaderboardEntry> m_SortedSolo;
    protected ref array<BattleRoyaleLeaderboardEntry> m_SortedGroup;
    protected bool m_SortValid;

    void BattleRoyaleLeaderboard()
    {
        m_Entries = new array<ref BattleRoyaleLeaderboardEntry>();
        m_ByUid = new map<string, BattleRoyaleLeaderboardEntry>();
        m_Recorded = new set<string>();
        m_GroupSizes = new map<string, int>();
        m_RequestCooldowns = new map<string, int>();
        m_SortedSolo = new array<BattleRoyaleLeaderboardEntry>();
        m_SortedGroup = new array<BattleRoyaleLeaderboardEntry>();

        m_FieldSize = 0;
        m_Ranking = false;
        m_Dirty = false;
        m_FlushDueMs = 0;
        m_SortValid = false;

        m_Settings = BattleRoyaleConfig.GetConfig().GetLeaderboardData();

        LoadFromDisk();
    }

    static BattleRoyaleLeaderboard GetInstance()
    {
        if (!m_Instance)
            m_Instance = new BattleRoyaleLeaderboard();

        return m_Instance;
    }

    protected void LoadFromDisk()
    {
        array<ref BattleRoyaleLeaderboardEntry> loaded = BattleRoyaleLeaderboardStore.Load(m_Settings);

        int count = loaded.Count();
        for (int i = 0; i < count; i++)
        {
            BattleRoyaleLeaderboardEntry entry = loaded.Get(i);
            if (!entry)
                continue;

            m_Entries.Insert(entry);
            m_ByUid.Set(entry.uid, entry);
        }

        m_SortValid = false;
    }

    /**
     *  Open the scoring window.
     *
     *  `field_groups` is the number of GROUPS in play, and `group_sizes` maps each player's uid to
     *  the size of their own group - both resolved by the caller in 5_Mission, where VigridPartyAPI
     *  is reachable. Passing NULL means "no party addon", i.e. everybody is solo.
     *
     *  Called at the moment gameplay begins rather than at lobby lock, because that is when deaths
     *  start counting. Snapshotting the field any earlier would let a lobby fill to 16 groups, have
     *  14 quit during the countdown, and still pay out as a 16-group win.
     */
    void BeginMatch(int field_groups, map<string, int> group_sizes)
    {
        m_FieldSize = field_groups;
        m_Ranking = true;
        m_Recorded.Clear();
        m_GroupSizes.Clear();

        if (group_sizes)
        {
            int count = group_sizes.Count();
            for (int i = 0; i < count; i++)
            {
                m_GroupSizes.Set(group_sizes.GetKey(i), group_sizes.GetElement(i));
            }
        }

        if (!m_Settings)
        {
            BattleRoyaleUtils.Warn("[Leaderboard] Match started with no settings loaded - nothing will be recorded");
            return;
        }

        bool ranked = BattleRoyaleScoring.IsRankedField(m_FieldSize, m_Settings);
        if (ranked)
            BattleRoyaleUtils.Info(string.Format("[Leaderboard] Match started, %1 groups in play - ranked", m_FieldSize));
        else
            BattleRoyaleUtils.Info(string.Format("[Leaderboard] Match started, %1 groups in play - UNRANKED (min_ranked_groups is %2)", m_FieldSize, m_Settings.min_ranked_groups));
    }

    //! Close the scoring window and force the ladder to disk.
    void EndMatch()
    {
        m_Ranking = false;
        Flush();
    }

    bool IsRanking()
    {
        return m_Ranking;
    }

    /**
     *  Record one player's finish. Safe to call for anyone, at any time, more than once.
     */
    void RecordExit(PlayerBase player)
    {
        if (!m_Settings)
            return;
        if (!m_Settings.enable_leaderboard)
            return;
        if (!m_Ranking)
            return;
        if (!player)
            return;

        string uid = player.player_steamid;
        if (uid == "" && player.GetIdentity())
            uid = player.GetIdentity().GetPlainId();
        if (uid == "")
            return;

        //--- RemovePlayer legitimately fires twice for a kill. Without this every death is counted
        //--- twice and every win is worth double.
        if (m_Recorded.Find(uid) != -1)
            return;
        m_Recorded.Insert(uid);

        bool ranked = BattleRoyaleScoring.IsRankedField(m_FieldSize, m_Settings);
        if (!ranked && !m_Settings.count_unranked_matches)
            return;

        int group_size = 1;
        if (m_GroupSizes.Contains(uid))
            group_size = m_GroupSizes.Get(uid);

        int board = BattleRoyaleScoring.BoardForGroupSize(group_size);
        int rank = player.GetBRPosition();
        int kills = player.br_kills;

        float points = 0.0;
        if (ranked)
            points = BattleRoyaleScoring.MatchPoints(rank, m_FieldSize, kills, m_Settings);

        BattleRoyaleLeaderboardEntry entry = GetOrCreate(uid);

        string display_name = player.player_name;
        if (display_name == "" && player.GetIdentity())
            display_name = player.GetIdentity().GetName();
        if (display_name != "")
            entry.name = display_name;

        entry.last_seen_hours = BattleRoyaleTime.NowHours();

        if (board == BR_LEADERBOARD_BOARD_GROUP)
        {
            entry.group_matches = entry.group_matches + 1;
            if (ranked)
            {
                entry.group_kills = entry.group_kills + kills;
                entry.group_points = entry.group_points + points;
            }
            if (ranked && rank == 1)
                entry.group_wins = entry.group_wins + 1;
        }
        else
        {
            entry.solo_matches = entry.solo_matches + 1;
            if (ranked)
            {
                entry.solo_kills = entry.solo_kills + kills;
                entry.solo_points = entry.solo_points + points;
            }
            if (ranked && rank == 1)
                entry.solo_wins = entry.solo_wins + 1;
        }

        BattleRoyaleUtils.Debug(string.Format("[Leaderboard] %1 finished at %2/%3 with %4 kills for %5 points (board %6)", uid, rank, m_FieldSize, kills, points, board));

        m_SortValid = false;
        MarkDirty(false);
    }

    void MarkDirty(bool flush_now)
    {
        //--- The debounce window opens at the FIRST unsaved change, so a burst of deaths at the end
        //--- of a round still lands on disk within one interval of the first of them.
        if (!m_Dirty)
            m_FlushDueMs = BattleRoyaleTime.NowMs() + BR_LEADERBOARD_FLUSH_DEBOUNCE_MS;

        m_Dirty = true;

        if (!flush_now)
            return;

        Flush();
    }

    //! Idempotent - repeated calls with nothing pending are free, which is what lets the match-end
    //! paths each call it without coordinating.
    void Flush()
    {
        if (!m_Dirty)
            return;

        BattleRoyaleLeaderboardStore.Save(m_Entries, m_Settings);
        m_Dirty = false;
    }

    //! Driven from BattleRoyaleServer.Update()'s existing 10 Hz gate.
    void Tick()
    {
        if (!m_Dirty)
            return;
        if (BattleRoyaleTime.NowMs() < m_FlushDueMs)
            return;

        Flush();
    }

    /**
     *  Answer a client's leaderboard request for one ladder.
     *
     *  Sends parallel primitive arrays rather than an array of structs - the convention everywhere
     *  else in this mod, and the only shape CF's RPC layer handles comfortably. Deliberately does
     *  not send uids: the client renders names only, so shipping SteamID64s would be pure liability.
     */
    void ServeRequest(PlayerIdentity sender, int board)
    {
        if (!sender)
            return;
        if (!m_Settings)
            return;

        string uid = sender.GetPlainId();

        int now_ms = BattleRoyaleTime.NowMs();
        if (m_RequestCooldowns.Contains(uid) && now_ms < m_RequestCooldowns.Get(uid))
            return;
        m_RequestCooldowns.Set(uid, now_ms + BR_LEADERBOARD_REQUEST_COOLDOWN_MS);

        int requested_board = BR_LEADERBOARD_BOARD_SOLO;
        if (board == BR_LEADERBOARD_BOARD_GROUP)
            requested_board = BR_LEADERBOARD_BOARD_GROUP;

        EnsureSorted();

        array<BattleRoyaleLeaderboardEntry> sorted = m_SortedSolo;
        if (requested_board == BR_LEADERBOARD_BOARD_GROUP)
            sorted = m_SortedGroup;

        int rows = m_Settings.leaderboard_top_rows;
        if (rows > BR_LEADERBOARD_MAX_ROWS)
            rows = BR_LEADERBOARD_MAX_ROWS;
        if (rows < 0)
            rows = 0;
        if (rows > sorted.Count())
            rows = sorted.Count();

        array<string> names = new array<string>();
        array<int> matches = new array<int>();
        array<int> wins = new array<int>();
        array<int> kills = new array<int>();
        array<int> points = new array<int>();

        for (int i = 0; i < rows; i++)
        {
            BattleRoyaleLeaderboardEntry row = sorted.Get(i);
            names.Insert(row.name);
            matches.Insert(BattleRoyaleLeaderboardStore.EntryMatches(row, requested_board));
            wins.Insert(BattleRoyaleLeaderboardStore.EntryWins(row, requested_board));
            kills.Insert(BattleRoyaleLeaderboardStore.EntryKills(row, requested_board));

            int row_points = Math.Round(BattleRoyaleLeaderboardStore.EntryPoints(row, requested_board));
            points.Insert(row_points);
        }

        //--- The requester's own standing, so someone outside the visible rows still sees where they
        //--- are. Rank 0 means "not on this ladder yet".
        int self_rank = 0;
        int self_wins = 0;
        int self_points = 0;

        int total = sorted.Count();
        for (int j = 0; j < total; j++)
        {
            BattleRoyaleLeaderboardEntry candidate = sorted.Get(j);
            if (candidate.uid != uid)
                continue;

            self_rank = j + 1;
            self_wins = BattleRoyaleLeaderboardStore.EntryWins(candidate, requested_board);
            self_points = Math.Round(BattleRoyaleLeaderboardStore.EntryPoints(candidate, requested_board));
            break;
        }

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetLeaderboard", new Param10<array<string>, array<int>, array<int>, array<int>, array<int>, int, int, int, int, int>( names, matches, wins, kills, points, requested_board, self_rank, self_wins, self_points, m_Settings.season ), true, sender );
    }

    protected BattleRoyaleLeaderboardEntry GetOrCreate(string uid)
    {
        if (m_ByUid.Contains(uid))
            return m_ByUid.Get(uid);

        ref BattleRoyaleLeaderboardEntry entry = new BattleRoyaleLeaderboardEntry();
        entry.uid = uid;
        entry.name = uid;

        m_Entries.Insert(entry);
        m_ByUid.Set(uid, entry);

        return entry;
    }

    /**
     *  Rebuild both sorted views if the table moved since last time.
     *
     *  A player only appears on a ladder they have actually played, otherwise the solo board would
     *  be padded with squad-only players sitting on zero.
     */
    protected void EnsureSorted()
    {
        if (m_SortValid)
            return;

        m_SortedSolo.Clear();
        m_SortedGroup.Clear();

        int count = m_Entries.Count();
        for (int i = 0; i < count; i++)
        {
            BattleRoyaleLeaderboardEntry entry = m_Entries.Get(i);
            if (!entry)
                continue;

            if (entry.solo_matches > 0)
                m_SortedSolo.Insert(entry);
            if (entry.group_matches > 0)
                m_SortedGroup.Insert(entry);
        }

        BattleRoyaleLeaderboardStore.SortByPoints(m_SortedSolo, BR_LEADERBOARD_BOARD_SOLO);
        BattleRoyaleLeaderboardStore.SortByPoints(m_SortedGroup, BR_LEADERBOARD_BOARD_GROUP);

        m_SortValid = true;
    }
}
#endif
