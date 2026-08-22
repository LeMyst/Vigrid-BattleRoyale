#ifdef SERVER
/**
 *  Battle Royale - persistence for leaderboard.json.
 *
 *  The ladder outlives the process on purpose: this server restarts between every match, so without
 *  this file a leaderboard would only ever show the match that just ended. That is the whole reason
 *  this exists.
 *
 *  Everything read off disk is treated as untrusted. A hand-edited or half-written file must never
 *  stop the server booting, so Load() repairs what it can, drops what it cannot, and falls back to
 *  the backup before it gives up.
 *
 *  Nothing in here logs through BattleRoyaleUtils.Error, and that is deliberate: Error routes to
 *  LogServerMessage(NONE, ...), which calls the engine's Error2() and raises a Virtual Machine
 *  Exception. It is a *fatal* logger. Using it on a recoverable path would kill the server over a
 *  corrupt leaderboard file, which is the exact opposite of what this class is for. Warn is the
 *  right severity for every failure the leaderboard can have - it is a cosmetic subsystem.
 *
 *  Ownership convention used throughout the leaderboard code:
 *    - array<ref BattleRoyaleLeaderboardEntry>  owns the entries (the live table, and the DTO)
 *    - array<BattleRoyaleLeaderboardEntry>      is a weak *view* used for sorting and for building
 *                                               an RPC payload, and never outlives the owner
 */
class BattleRoyaleLeaderboardStore
{
    /**
     *  Read leaderboard.json and return the surviving entries.
     *
     *  In order: parse (falling back to the backup), season rollover, then pruning -
     *    - entries with no uid
     *    - duplicate uids, first occurrence wins
     *    - entries not seen for longer than leaderboard_ttl_days
     *    - the lowest-scoring entries beyond leaderboard_max_entries
     */
    static array<ref BattleRoyaleLeaderboardEntry> Load(BattleRoyaleLeaderboardData settings)
    {
        array<ref BattleRoyaleLeaderboardEntry> result = new array<ref BattleRoyaleLeaderboardEntry>();

        if (!settings)
        {
            BattleRoyaleUtils.Warn("[Leaderboard] No settings available, starting empty");
            return result;
        }

        if (!FileExist(BATTLEROYALE_LEADERBOARD_FILE))
        {
            //--- Deliberately does NOT fall back to the backup here. A missing primary is either
            //--- first boot or an admin deleting the ladder on purpose; silently resurrecting it
            //--- from .bak would be the wrong answer to both.
            BattleRoyaleUtils.Info("[Leaderboard] No leaderboard.json yet - starting with an empty ladder");
            return result;
        }

        BattleRoyaleLeaderboardFile store = ParseStoreFile(BATTLEROYALE_LEADERBOARD_FILE);
        if (!store)
        {
            BattleRoyaleUtils.Warn("[Leaderboard] leaderboard.json unreadable, trying the backup");
            store = ParseStoreFile(BATTLEROYALE_LEADERBOARD_BACKUP);
        }

        if (!store)
        {
            //--- Losing the ladder beats refusing to start. The next flush overwrites the file.
            BattleRoyaleUtils.Warn("[Leaderboard] Neither leaderboard.json nor its backup could be read, starting empty");
            return result;
        }

        if (!store.entries)
        {
            BattleRoyaleUtils.Warn("[Leaderboard] leaderboard.json has no entries array, starting empty");
            return result;
        }

        //--- Season rollover. The admin bumps `season` in leaderboard_settings.json and the next
        //--- boot retires the old ladder; the per-match restart makes that near-instant.
        if (store.season != settings.season)
        {
            ArchiveSeason(store.season);
            BattleRoyaleUtils.Info(string.Format("[Leaderboard] Season %1 retired, starting season %2 empty", store.season, settings.season));
            return result;
        }

        int now_hours = BattleRoyaleTime.NowHours();
        int ttl_hours = settings.leaderboard_ttl_days * 24;
        set<string> claimed = new set<string>();

        int entry_count = store.entries.Count();
        for (int i = 0; i < entry_count; i++)
        {
            BattleRoyaleLeaderboardEntry entry = store.entries.Get(i);
            if (!entry)
                continue;
            if (entry.uid == "")
                continue;
            if (claimed.Find(entry.uid) != -1)
            {
                BattleRoyaleUtils.Warn("[Leaderboard] uid " + entry.uid + " appears more than once, keeping the first");
                continue;
            }
            if (ttl_hours > 0 && (now_hours - entry.last_seen_hours) > ttl_hours)
                continue;

            claimed.Insert(entry.uid);
            result.Insert(entry);
        }

        Truncate(result, settings);

        BattleRoyaleUtils.Info(string.Format("[Leaderboard] Loaded %1 entries for season %2", result.Count(), settings.season));
        return result;
    }

    /**
     *  Write the table back to disk.
     *
     *  JsonFileLoader.SaveFile is not atomic, so the previous file is copied aside first: a crash
     *  mid-write would otherwise truncate the only copy and lose every match ever played. With the
     *  backup, the worst case is bounded to one flush interval.
     */
    static void Save(array<ref BattleRoyaleLeaderboardEntry> entries, BattleRoyaleLeaderboardData settings)
    {
        if (!entries)
            return;
        if (!settings)
            return;

        BattleRoyaleLeaderboardFile store = new BattleRoyaleLeaderboardFile();
        store.version = 1;
        store.season = settings.season;
        store.saved_at = BattleRoyaleTime.NowSeconds();

        int count = entries.Count();
        for (int i = 0; i < count; i++)
        {
            BattleRoyaleLeaderboardEntry entry = entries.Get(i);
            if (!entry)
                continue;
            if (entry.uid == "")
                continue;

            store.entries.Insert(entry);
        }

        if (FileExist(BATTLEROYALE_LEADERBOARD_FILE))
            CopyFile(BATTLEROYALE_LEADERBOARD_FILE, BATTLEROYALE_LEADERBOARD_BACKUP);

        string error_message;
        if (!JsonFileLoader<BattleRoyaleLeaderboardFile>.SaveFile(BATTLEROYALE_LEADERBOARD_FILE, store, error_message))
        {
            BattleRoyaleUtils.Warn("[Leaderboard] Failed to write leaderboard.json: " + error_message);
            return;
        }

        BattleRoyaleUtils.Debug(string.Format("[Leaderboard] Wrote %1 entries to disk", store.entries.Count()));
    }

    /**
     *  Points for one entry on one ladder. Any board value other than SOLO or GROUP means "both
     *  combined", which is what the storage cap ranks on - a player strong in solo but who has never
     *  played grouped should not be evicted by a mediocre squad player.
     */
    static float EntryPoints(BattleRoyaleLeaderboardEntry entry, int board)
    {
        if (!entry)
            return 0.0;
        if (board == BR_LEADERBOARD_BOARD_SOLO)
            return entry.solo_points;
        if (board == BR_LEADERBOARD_BOARD_GROUP)
            return entry.group_points;

        return entry.solo_points + entry.group_points;
    }

    static int EntryWins(BattleRoyaleLeaderboardEntry entry, int board)
    {
        if (!entry)
            return 0;
        if (board == BR_LEADERBOARD_BOARD_SOLO)
            return entry.solo_wins;
        if (board == BR_LEADERBOARD_BOARD_GROUP)
            return entry.group_wins;

        return entry.solo_wins + entry.group_wins;
    }

    static int EntryMatches(BattleRoyaleLeaderboardEntry entry, int board)
    {
        if (!entry)
            return 0;
        if (board == BR_LEADERBOARD_BOARD_SOLO)
            return entry.solo_matches;
        if (board == BR_LEADERBOARD_BOARD_GROUP)
            return entry.group_matches;

        return entry.solo_matches + entry.group_matches;
    }

    static int EntryKills(BattleRoyaleLeaderboardEntry entry, int board)
    {
        if (!entry)
            return 0;
        if (board == BR_LEADERBOARD_BOARD_SOLO)
            return entry.solo_kills;
        if (board == BR_LEADERBOARD_BOARD_GROUP)
            return entry.group_kills;

        return entry.solo_kills + entry.group_kills;
    }

    /**
     *  Sort a weak view best-first: points desc, then wins desc, ties keeping their original order.
     *
     *  Merge sort rather than the obvious repeated-scan, because the table is capped at 5000 rows
     *  and an O(n^2) sort there is 25M comparisons at boot. This is ~60k.
     */
    static void SortByPoints(array<BattleRoyaleLeaderboardEntry> view, int board)
    {
        if (!view)
            return;

        int count = view.Count();
        if (count < 2)
            return;

        array<BattleRoyaleLeaderboardEntry> scratch = new array<BattleRoyaleLeaderboardEntry>();
        scratch.Resize(count);

        MergeSortRange(view, scratch, 0, count - 1, board);
    }

    //! True when `a` outranks `b`. Strict, so equal entries keep their original relative order.
    static bool IsBefore(BattleRoyaleLeaderboardEntry a, BattleRoyaleLeaderboardEntry b, int board)
    {
        float points_a = EntryPoints(a, board);
        float points_b = EntryPoints(b, board);
        if (points_a > points_b)
            return true;
        if (points_a < points_b)
            return false;

        int wins_a = EntryWins(a, board);
        int wins_b = EntryWins(b, board);
        if (wins_a > wins_b)
            return true;

        return false;
    }

    //--- Parse one file, returning NULL when it cannot be read. Shared by the primary and backup
    //--- paths so the two can never drift apart.
    //---
    //--- NOT named ReadFile: that is a vanilla global proto (ensystem.c), and an unqualified call to
    //--- a member sharing a vanilla global's name resolves to the wrong thing in EnfusionScript.
    private static BattleRoyaleLeaderboardFile ParseStoreFile(string path)
    {
        if (!FileExist(path))
            return NULL;

        BattleRoyaleLeaderboardFile store = new BattleRoyaleLeaderboardFile();
        string error_message;

        if (!JsonFileLoader<BattleRoyaleLeaderboardFile>.LoadFile(path, store, error_message))
        {
            BattleRoyaleUtils.Warn("[Leaderboard] " + path + " could not be read: " + error_message);
            return NULL;
        }

        return store;
    }

    //--- Copy the finished ladder aside under its season number. Best-effort: if the copy fails the
    //--- season still rolls over, because refusing to start a new season over a file error would be
    //--- worse than losing the archive.
    private static void ArchiveSeason(int season)
    {
        string archive_path = string.Format(BATTLEROYALE_LEADERBOARD_ARCHIVE_FMT, season);

        if (!CopyFile(BATTLEROYALE_LEADERBOARD_FILE, archive_path))
        {
            BattleRoyaleUtils.Warn("[Leaderboard] Failed to archive season " + season + " to " + archive_path);
            return;
        }

        BattleRoyaleUtils.Info("[Leaderboard] Archived season " + season + " to " + archive_path);
    }

    //--- Enforce leaderboard_max_entries, dropping the lowest combined scores first.
    private static void Truncate(array<ref BattleRoyaleLeaderboardEntry> entries, BattleRoyaleLeaderboardData settings)
    {
        int cap = settings.leaderboard_max_entries;
        if (cap <= 0)
            return;
        if (entries.Count() <= cap)
            return;

        array<BattleRoyaleLeaderboardEntry> view = new array<BattleRoyaleLeaderboardEntry>();
        int total = entries.Count();
        for (int i = 0; i < total; i++)
        {
            view.Insert(entries.Get(i));
        }

        //--- -1 is neither SOLO nor GROUP, so this ranks on the combined total.
        SortByPoints(view, -1);

        set<string> keep = new set<string>();
        for (int k = 0; k < cap; k++)
        {
            keep.Insert(view.Get(k).uid);
        }

        //--- Walked backwards: RemoveOrdered shifts every later element down, so going forwards
        //--- would skip the entry that slid into the freed slot.
        for (int j = entries.Count() - 1; j >= 0; j--)
        {
            if (keep.Find(entries.Get(j).uid) == -1)
                entries.RemoveOrdered(j);
        }

        BattleRoyaleUtils.Info(string.Format("[Leaderboard] Pruned to leaderboard_max_entries (%1), dropped %2", cap, total - entries.Count()));
    }

    private static void MergeSortRange(array<BattleRoyaleLeaderboardEntry> view, array<BattleRoyaleLeaderboardEntry> scratch, int left, int right, int board)
    {
        if (left >= right)
            return;

        int mid = left + (right - left) / 2;
        MergeSortRange(view, scratch, left, mid, board);
        MergeSortRange(view, scratch, mid + 1, right, board);

        int i = left;
        int j = mid + 1;
        int k = left;

        while (i <= mid && j <= right)
        {
            //--- Take the right side only when it strictly outranks the left, which is what keeps
            //--- the sort stable.
            if (IsBefore(view.Get(j), view.Get(i), board))
            {
                scratch.Set(k, view.Get(j));
                j++;
            }
            else
            {
                scratch.Set(k, view.Get(i));
                i++;
            }
            k++;
        }

        while (i <= mid)
        {
            scratch.Set(k, view.Get(i));
            i++;
            k++;
        }

        while (j <= right)
        {
            scratch.Set(k, view.Get(j));
            j++;
            k++;
        }

        for (int c = left; c <= right; c++)
        {
            view.Set(c, scratch.Get(c));
        }
    }
}
#endif
