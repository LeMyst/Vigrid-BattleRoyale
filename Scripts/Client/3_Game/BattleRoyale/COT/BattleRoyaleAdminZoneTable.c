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

    ref array<ref BattleRoyaleAdminZoneRow> rows = new array<ref BattleRoyaleAdminZoneRow>();
}
