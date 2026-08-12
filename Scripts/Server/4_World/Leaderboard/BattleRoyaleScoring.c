#ifdef SERVER
/**
 *  Battle Royale - leaderboard scoring math. Pure functions: no I/O, no state, no party symbols.
 *
 *  Deliberately free of VigridPartyAPI. That class lives in Party's own 4_World and this module has
 *  no declared dependency on that PBO, so group counts are resolved up in 5_Mission (where every
 *  other #ifdef VIGRID_PARTY call site in this mod already lives) and arrive here as plain ints.
 *
 *  See BattleRoyaleLeaderboardData for the model and the reasoning behind each knob.
 *
 *  The one trap worth restating: `rank` is PlayerBase.br_position, which is GROUPS REMAINING when
 *  the player left, not a unique per-player rank. Every member of a winning 4-stack holds 1; every
 *  member of a team wiped when 7 groups were left holds 7. That is exactly right for group-weighted
 *  scoring, but it means ranks are shared and you can never treat them as a permutation of 1..N.
 */
class BattleRoyaleScoring
{
    /**
     *  Does a match of this size count at all? Below the floor nothing is awarded - not points, not
     *  kills, and (unless count_unranked_matches is on) not even a match. This is the anti-farm
     *  gate: without it, a run of tiny 4am lobbies beats a full one under every weighting curve.
     */
    static bool IsRankedField(int field_groups, BattleRoyaleLeaderboardData settings)
    {
        if (!settings)
            return false;
        if (!settings.enable_leaderboard)
            return false;
        //--- N < 2 is not a contest, and it would also put Log2 at or below 1 and make the
        //--- (N - 1) divisor zero. Guarded here so every caller inherits it.
        if (field_groups < 2)
            return false;
        if (field_groups < settings.min_ranked_groups)
            return false;

        return true;
    }

    /**
     *  What a win in a field of this many groups is worth, before placement.
     *
     *  At the log2 default with scale 25: N=2 -> 25, N=5 -> 58, N=15 -> 98, N=60 -> 148. Doubling
     *  the field always adds a flat +25, which is what keeps a single lucky full-lobby win from
     *  being unassailable the way the linear curve makes it.
     *
     *  Flat mode means literally "every ranked win is worth field_weight_scale", ignoring N.
     */
    static float FieldWeight(int field_groups, BattleRoyaleLeaderboardData settings)
    {
        if (!settings)
            return 0.0;
        if (field_groups < 2)
            return 0.0;

        float n = field_groups;
        float raw = 1.0;

        //--- No ternary in EnfusionScript, and a switch on a config int is no clearer than this.
        if (settings.field_weight_mode == BR_LEADERBOARD_WEIGHT_LOG2)
            raw = Math.Log2(n);
        if (settings.field_weight_mode == BR_LEADERBOARD_WEIGHT_SQRT)
            raw = Math.Sqrt(n - 1.0);
        if (settings.field_weight_mode == BR_LEADERBOARD_WEIGHT_LINEAR)
            raw = n - 1.0;

        float weight = settings.field_weight_scale * raw;

        return Math.Clamp(weight, settings.field_weight_min, settings.field_weight_max);
    }

    /**
     *  Fraction of the field weight earned by finishing at `rank`: 1.0 for the winner, decaying to
     *  placement_floor for the first group out.
     */
    static float PlacementCredit(int rank, int field_groups, BattleRoyaleLeaderboardData settings)
    {
        if (!settings)
            return 0.0;
        if (field_groups < 2)
            return 0.0;

        int placement = rank;
        //--- br_position initialises to -1 and stays there for anyone the 5s OnPlayerCountChanged
        //--- timer never reached. Treat an unknown finish as dead last rather than as a win.
        if (placement < 1)
            placement = field_groups;
        //--- Defensive: a late joiner could in principle leave with more groups remaining than the
        //--- field started with. Never let that produce negative progress.
        if (placement > field_groups)
            placement = field_groups;

        //--- These MUST be float locals before the divide. With int operands `(N - rank) / (N - 1)`
        //--- truncates to 0 or 1 and silently flattens the entire curve - the single most likely
        //--- bug in this file.
        float outlasted = field_groups - placement;
        float contested = field_groups - 1;
        float progress = outlasted / contested;
        progress = Math.Clamp(progress, 0.0, 1.0);

        float shaped = Math.Pow(progress, settings.placement_exponent);
        float floor_credit = Math.Clamp(settings.placement_floor, 0.0, 1.0);

        return floor_credit + (1.0 - floor_credit) * shaped;
    }

    /**
     *  Total points for one player's match.
     *
     *  Placement is field-weighted, kills are flat. Weighting kills too would count the field size
     *  twice, and would mean a kill in a big lobby was worth more than one in a small lobby - which
     *  is not true of anything the player actually did.
     */
    static float MatchPoints(int rank, int field_groups, int kills, BattleRoyaleLeaderboardData settings)
    {
        if (!IsRankedField(field_groups, settings))
            return 0.0;

        float weight = FieldWeight(field_groups, settings);
        float credit = PlacementCredit(rank, field_groups, settings);
        float placement_points = weight * credit;

        float kill_count = kills;
        if (kill_count < 0.0)
            kill_count = 0.0;

        return placement_points + kill_count * settings.kill_points;
    }

    //! Which ladder a result belongs to, from the player's own group size at match start.
    static int BoardForGroupSize(int group_size)
    {
        if (group_size > 1)
            return BR_LEADERBOARD_BOARD_GROUP;

        return BR_LEADERBOARD_BOARD_SOLO;
    }
}
#endif
