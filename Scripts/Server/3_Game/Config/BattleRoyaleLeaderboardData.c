#ifdef SERVER
/**
 *  Settings for the persistent leaderboard - $profile:Vigrid-BattleRoyale\leaderboard_settings.json
 *
 *  Only the knobs live here. The leaderboard *data* is a standalone store
 *  (BattleRoyaleLeaderboardStore) rather than another entry in this registry, because
 *  BattleRoyaleConfig.Load() re-Save()s every entry at boot and applies $mission: overrides -
 *  correct for admin settings, wrong for mutable player records.
 *
 *  Like server_settings.json this deliberately takes no mission override: GetMissionPath() is left
 *  at the base's "", so BattleRoyaleConfig.Load() skips the mission pass. A mission pack silently
 *  rewriting the scoring curve would be a bug factory.
 *
 *  ---------------------------------------------------------------------------------------------
 *  THE SCORING MODEL
 *
 *  The problem this exists to solve: a win against 60 contenders is not the same achievement as a
 *  win against 5, and a 4-stack win is not the same as a solo win. Two independent axes fix that.
 *
 *  1. Field-size weighting. With N equal contenders the chance of winning is 1/N, so a win is worth
 *     log2(N) - `weight` below. N is counted in GROUPS, never players: a 60-player lobby made of 15
 *     squads is a 1-in-15 win, not 1-in-60. (br_position is already group-based, so the two agree.)
 *
 *  2. Separate solo and group ladders, split by the player's own group size at match start. Parties
 *     here are opt-in within a match rather than a playlist, so one match can feed both boards.
 *
 *  Per player, per match:
 *
 *      weight   = Clamp(field_weight_scale * log2(N), field_weight_min, field_weight_max)
 *      progress = (N - rank) / (N - 1)                       // 1.0 winner .. 0.0 last
 *      credit   = placement_floor + (1 - placement_floor) * progress ^ placement_exponent
 *      points   = weight * credit + kill_points * kills
 *
 *  Placement points are field-weighted (surviving to 2nd of 30 groups should beat winning a 3-group
 *  match); kill points are flat, because a kill is a kill and weighting them would count the field
 *  twice. Both are suppressed entirely below min_ranked_groups.
 */
class BattleRoyaleLeaderboardData: BattleRoyaleDataBase
{
    int version = 1;  // Config version

    // Master switch. When false nothing is recorded, nothing is written, and the client menu shows
    // an empty board.
    bool enable_leaderboard = true;

    // Current season. Bump this to retire the standing ladder: on the next boot the store archives
    // leaderboard.json to leaderboard_s<old>.json and starts empty. The server already restarts
    // between matches, so editing this number is the whole "wipe the ladder" workflow - no admin UI.
    int season = 1;

    // Matches with fewer than this many groups award nothing at all - no points, no kills, not even
    // a match count. THIS, not the choice of curve, is the real anti-farm defence: without it a
    // handful of 2-player matches at 4am out-earn a full lobby under any weighting.
    int min_ranked_groups = 4;

    // Whether an unranked match (below min_ranked_groups) still bumps the player's match counter.
    // Off by default so the average-points figure stays honest.
    bool count_unranked_matches = false;

    // Field-size curve. 0 flat, 1 log2, 2 sqrt, 3 linear - see BattleRoyaleScoring.FieldWeight.
    //   log2   N=2 -> 25, N=15 -> 98, N=60 -> 148   (a 60-vs-5 spread of 2.5x)
    //   sqrt   N=2 -> 25, N=15 -> 93, N=60 -> 192   (3.8x)
    //   linear N=2 -> 25, N=15 -> 350, N=60 -> 1475 (14.8x - one lucky full lobby becomes
    //                                                unassailable, which is why log2 is the default)
    int field_weight_mode = 1;

    // Multiplier on the curve. At the log2 default this is "points added per doubling of the field",
    // so a full-lobby win sits near 150 and a minimum-size one near 25.
    float field_weight_scale = 25.0;

    // Clamp on the finished weight, guarding against an absurd hand-edited curve or field size.
    float field_weight_min = 1.0;
    float field_weight_max = 200.0;

    // How sharply points concentrate at the top. 1.0 is linear in placement; 2.0 (default) makes the
    // last few groups standing worth substantially more than mid-table.
    float placement_exponent = 2.0;

    // Credit for finishing dead last, as a fraction of the full weight. 0.0 means the first player
    // out of a ranked match scores nothing but their kills.
    float placement_floor = 0.0;

    // Flat points per kill, NOT field-weighted. Keep this small relative to a win or the board
    // stops being a placement board: at the defaults a 6-kill 5th place still loses to a clean win.
    float kill_points = 5.0;

    // Rows sent to a client per board. Hard-clamped to BR_LEADERBOARD_MAX_ROWS in code whatever
    // this says - there is no RPC chunking anywhere in this mod.
    int leaderboard_top_rows = 25;

    // Drop entries not seen for this long. 0 disables TTL pruning.
    int leaderboard_ttl_days = 90;

    // Hard cap on stored rows; the lowest-scoring are dropped first. Bounds the file size on a
    // long-lived server.
    int leaderboard_max_entries = 5000;

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "leaderboard_settings.json";
    }

    override void Load()
    {
        string errorMessage;
        // Load from profile folder
        if (FileExist(GetProfilePath()))
        {
            if (!JsonFileLoader<BattleRoyaleLeaderboardData>.LoadFile(GetProfilePath(), this, errorMessage))
                ErrorEx(errorMessage);
        }

        // Run the upgrade function here to avoid overrides from mission folder
        Upgrade();

        // No load from mission folder for this one
    }

    override void Save()
    {
        string errorMessage;
        if (!JsonFileLoader<BattleRoyaleLeaderboardData>.SaveFile(GetProfilePath(), this, errorMessage))
            ErrorEx(errorMessage);
    }

    override void Upgrade()
    {
        //--- Nothing to migrate yet. Note that simply ADDING a field needs no bump: Config.Load()
        //--- re-saves after reading, so new members appear in existing profile JSONs on next boot.
        //--- Only a changed meaning for an existing value needs a version here.
        //---
        //--- When that day comes: any array migration must be guarded on Count() == 0. Both
        //--- BattleRoyaleGameData and BattleRoyaleLobbyData Insert() onto a field initialiser in
        //--- their Upgrade(), so their arrays double every boot. Do not copy that.
    }
};
#endif
