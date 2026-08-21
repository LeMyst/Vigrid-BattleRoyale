/**
 *  The generated circle chain, as data rather than as the LOG_ZONE_TABLE log dump - which is behind
 *  DIAG_DEVELOPER and so unreachable on a live server.
 *
 *  UNGUARDED and JSON-serialized, same reasoning as BattleRoyaleAdminStatus.c.
 *
 *  ⚠️ ROWS ARE IN PLAY ORDER, NOT ARRAY ORDER, and the difference is the single most surprising fact
 *  in the zone subsystem. Generation runs SMALLEST FIRST: `m_PlayAreas[0]` is the tight FINAL circle
 *  and each later index is a larger circle containing it. Dumping the array as-is shows the match
 *  backwards. The builder reverses it and `settings_index` keeps the original index so a reading can
 *  still be matched against zone_settings.json.
 */
class BattleRoyaleAdminZoneRow
{
    //--- Index into zone_settings.json's static_sizes / static_timers / min_players. NOT the row's
    //--- position in the table: the table is in play order and this is not.
    int   settings_index = -1;

    //--- 1 = the first circle played, counting up. What an operator means by "circle 3".
    int   play_order = 0;

    float radius = 0;
    float center_x = 0;
    float center_z = 0;

    //--- Authored round length from static_timers, before any offset.
    int   timer_s = 0;

    //--- Extra seconds CommitChain granted this circle because it landed far from its parent, or
    //--- because it grew. 0 for most circles.
    int   offset_s = 0;

    //--- What derive_timers_from_geometry WOULD give this round. Computed unconditionally by
    //--- CommitChain, so this column is populated and worth showing even when the setting is off -
    //--- which is precisely when an operator wants to know what turning it on would do.
    int   derived_timer_s = 0;

    //--- What this round would cost if it were the OPENING one - a loot allowance plus the sprint a
    //--- spawned player still owes inside the circle, with no inbound travel. Which round that is
    //--- depends on the player count, so it is computed for every circle and shown for every circle.
    int   opening_timer_s = 0;

    //--- POIs inside this circle - villages, towns, the CfgWorlds names the final circle can seed on,
    //--- after the avoid lists. -1 means "not counted", which is a different thing from a circle over
    //--- empty ground. This is the loot-density input to the derived min_players beside it.
    int   poi_count = -1;

    //--- How many players this circle is rated for. `min_players` is the AUTHORED zone_settings.json
    //--- entry and `derived_min_players` is what derive_zone_ladder computes; the table's
    //--- `derive_ladder` flag says which of the two the match is using. Both are shown either way, for
    //--- the same reason derived_timer_s is - an operator consults the column to decide.
    int   min_players = 0;
    int   derived_min_players = -1;

    //--- Committed radius minus the admin's static size, i.e. what allow_zone_size_flex spent here.
    //--- Rare at stock settings (~0.1% of placements on ChernarusPlus), so a non-zero value is worth
    //--- seeing.
    float growth_m = 0;

    //--- Skipped circles are generated and never played - the dynamic starting zone drops the
    //--- largest ones. They are listed rather than hidden so the table matches zone_settings.json,
    //--- but they are marked, because a skipped circle is not on anybody's map.
    bool  skipped = false;

    //--- The circle currently in play, if any.
    bool  current = false;
}

class BattleRoyaleAdminZoneTable
{
    bool valid = false;

    int  generation_seed = 0;
    int  num_zones = 0;

    //--- Whether the derived_timer_s column is what the match is actually using, or only what it
    //--- would use. Without this the column is ambiguous in the one direction that matters.
    bool derive_timers = false;
    bool allow_flex = false;

    //--- Whether the derived_min_players column is what the match is actually using, and whether the
    //--- starting tier is being moved to fit a duration target. Same ambiguity the flag above removes.
    bool derive_ladder = false;
    bool bound_duration = false;

    ref array<ref BattleRoyaleAdminZoneRow> rows = new array<ref BattleRoyaleAdminZoneRow>();
}
