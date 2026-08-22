#ifdef SERVER
/**
 *  Every lootable building on the map, scanned once and cached in scan_cache.json.
 *
 *  WHY THIS EXISTS. The derived ladder (#284) rates a circle by how much loot it holds, and POI count
 *  does not measure loot - correlation 0.15-0.29 against the Central Economy's own loot points, where
 *  the count of buildings inside the circle scores 0.993-0.995. The first implementation scanned the
 *  largest circle at generation time, which worked but had two faults this class fixes:
 *
 *    1. IT COST 8.7 s ON EVERY BOOT. 167 probes over one 3375 m circle, measured on ChernarusPlus,
 *       and nothing was reusable because the circles move every match.
 *    2. IT COULD ONLY SEE RELATIVE POVERTY. With no map-wide figure the reference density had to be
 *       the largest circle's own, which pins that circle at factor 1.0 by construction - so a chain
 *       landing in a uniformly loot-poor region rated exactly like one in a rich region. Measured:
 *       opening circles holding 1316, 2744 and 3174 buildings were all rated for 33 players.
 *
 *  Scanning the whole world once and caching it answers both: later boots pay a file read, and the
 *  map-wide density is a real absolute yardstick, so a genuinely poor region now buys a bigger opening
 *  circle and therefore an extra shrink.
 *
 *  Measured on ChernarusPlus: 961 probes, 39.9 s, 14,141 buildings, 59.9 per km2. Every later boot
 *  reads it back instead.
 *
 *  MEASURED PROPERTIES OF THE UNDERLYING QUERY are in BattleRoyalePOIResolver's header - statics do
 *  come back from GetObjectsAtPosition at arbitrary range, the object tree is fully populated at
 *  MissionServer.OnInit, and cost is first-touch per region. This class is a second consumer of that
 *  same finding, and deliberately borrows the resolver's collector rather than re-deriving the two
 *  rules that make a count meaningful (Building.Cast, and poi_scan_exclude).
 */
class BattleRoyaleBuildingCensus
{
    //--- Bump when the SEMANTICS of a stored count change. The cell size is in the signature instead,
    //--- because changing it is a tuning decision rather than a format change.
    static const int CACHE_VERSION = 1;

    static ref array<float> s_Positions;
    static bool s_Ready;
    static int s_ScanMs;
    static int s_Probes;
    static bool s_FromCache;

    static bool IsReady()
    {
        return s_Ready;
    }

    static int GetBuildingCount()
    {
        if (!s_Positions)
            return 0;

        return s_Positions.Count() / 2;
    }

    //--- Buildings per SQUARE METRE across the whole world. The absolute yardstick every circle's own
    //--- density is compared against.
    static float GetMapDensity()
    {
        float world_size = 0;

        if (GetGame() && GetGame().GetWorld())
            world_size = GetGame().GetWorld().GetWorldSize();

        if (world_size <= 0)
            return 0;

        return GetBuildingCount() / (world_size * world_size);
    }

    //--- Exact, not gridded: the positions are kept, so a circle of any radius gets a true count.
    //--- 11k buildings against six circles is 66k float comparisons, which is nothing next to the
    //--- native scan it replaces.
    static int CountInCircle(vector centre, float radius)
    {
        int i;
        int pairs;
        int hits = 0;
        float dx;
        float dz;
        float r2 = radius * radius;
        float cx = centre[0];
        float cz = centre[2];

        if (!s_Positions || radius <= 0)
            return 0;

        pairs = s_Positions.Count() / 2;
        for (i = 0; i < pairs; i++)
        {
            //--- Each read on its own line before it is used, per the rule in
            //--- BattleRoyaleZone.ComputeAllowRadii: an array read sharing an expression with a call
            //--- has been measured in this codebase to return another array's contents.
            dx = s_Positions.Get(i * 2) - cx;
            dz = s_Positions.Get((i * 2) + 1) - cz;

            if (((dx * dx) + (dz * dz)) <= r2)
                hits++;
        }

        return hits;
    }

    /**
     *  Load the cache, or scan the world and write one.
     *
     *  Call once, from BattleRoyaleServer.Init(), and ONLY when something needs it - the scan is the
     *  most expensive thing the mod does at boot, and a server with derive_zone_ladder off must not
     *  pay for a number nothing reads.
     */
    static void Init()
    {
        string world_name;
        string signature;
        int started_ms;

        if (s_Ready)
            return;

        s_Positions = new array<float>;
        s_Ready = false;
        s_FromCache = false;

        if (!GetGame())
            return;

        world_name = GetGame().GetWorldName();
        world_name.ToLower();
        signature = BuildSignature();

        if (LoadCache(world_name, signature))
        {
            s_Ready = true;
            s_FromCache = true;
            BattleRoyaleUtils.Info("[BuildingCensus] cache hit for " + world_name + " - " + GetBuildingCount() + " buildings, no scan needed.");
            return;
        }

        BattleRoyaleUtils.Info("[BuildingCensus] no usable cache for " + world_name + " - scanning the whole map once. This takes tens of seconds; every later boot reads the file instead.");

        started_ms = GetGame().GetTime();
        ScanWorld();
        s_ScanMs = GetGame().GetTime() - started_ms;

        s_Ready = (GetBuildingCount() > 0);

        if (!s_Ready)
        {
            //--- Not theoretical: if GetObjectsAtPosition ever stops returning statics at range, a
            //--- census of zero would rate every circle identically and silently. Say so, write
            //--- nothing, and let the caller fall back.
            BattleRoyaleUtils.Warn("[BuildingCensus] scanned " + s_Probes + " probes and found NOTHING. The loot-density term will fall back to POI counts. Check that GetObjectsAtPosition returns statics on this map.");
            return;
        }

        BattleRoyaleUtils.Info("[BuildingCensus] scanned " + s_Probes + " probes in " + s_ScanMs + " ms - " + GetBuildingCount() + " lootable buildings, " + (GetMapDensity() * 1000000) + " per km2.");
        SaveCache(world_name, signature);
    }

    /**
     *  Tile the world with squares and probe each one's circumcircle.
     *
     *  ⚠️ A LATTICE OF SMALL PROBES, NEVER ONE BIG QUERY. GetObjectsAtPosition fills an array with
     *  EVERY object in range - trees, bushes and rocks outnumber buildings by orders of magnitude -
     *  so a world-sized query would hand back a seven-figure array to walk.
     *
     *  ⚠️ EVERY BUILDING IS COUNTED EXACTLY ONCE. Probe circles must overlap to cover a plane, so a
     *  building is kept only when it falls inside its own probe's SQUARE, which makes the cells a
     *  partition. A plain radius test would double-count the seams, and seams fall in dense town
     *  centres as readily as anywhere else.
     */
    protected static void ScanWorld()
    {
        int cell_x;
        int cell_z;
        int cells;
        int i;
        int found_here;
        float world_size;
        float cell;
        float probe_r;
        float half;
        float bx;
        float bz;
        vector probe = "0 0 0";
        vector building;
        array<vector> found;

        s_Probes = 0;

        world_size = GetGame().GetWorld().GetWorldSize();
        if (world_size <= 0)
            return;

        cell = BR_ZONE_CENSUS_CELL_M;
        probe_r = cell * 0.7072;              //--- circumradius of the square, so the cell is covered
        half = cell * 0.5;

        cells = Math.Ceil(world_size / cell);

        //--- The probe budget. Skipping is the right failure here rather than scanning anyway: the
        //--- caller falls back to POI counts and says so, whereas an unbounded scan on a huge terrain
        //--- would stall boot with no way for an operator to tell what was happening.
        if ((cells * cells) > BR_ZONE_CENSUS_MAX_CELLS)
        {
            BattleRoyaleUtils.Warn("[BuildingCensus] a " + world_size + " m world needs " + (cells * cells) + " probes at " + cell + " m cells, over the " + BR_ZONE_CENSUS_MAX_CELLS + " cap - skipping the census. The ladder will fall back to POI counts. Raise BR_ZONE_CENSUS_CELL_M for this map.");
            return;
        }

        for (cell_z = 0; cell_z < cells; cell_z++)
        {
            for (cell_x = 0; cell_x < cells; cell_x++)
            {
                probe[0] = (cell_x * cell) + half;
                probe[2] = (cell_z * cell) + half;

                found = new array<vector>();
                BattleRoyalePOIResolver.CollectBuildingsAt(probe, probe_r, found);
                s_Probes++;

                found_here = found.Count();
                for (i = 0; i < found_here; i++)
                {
                    building = found.Get(i);
                    bx = building[0];
                    bz = building[2];

                    //--- THE PARTITION TEST. Half-open on both axes, so a building sitting exactly on
                    //--- a shared edge belongs to one cell and not two.
                    if (bx < (probe[0] - half) || bx >= (probe[0] + half))
                        continue;
                    if (bz < (probe[2] - half) || bz >= (probe[2] + half))
                        continue;

                    s_Positions.Insert(bx);
                    s_Positions.Insert(bz);
                }
            }
        }
    }

    //--- Everything that would change a stored count. The exclusion list is in here because it decides
    //--- what CollectBuildingsAt keeps, so editing pois_settings.poi_scan_exclude must invalidate this
    //--- cache and not quietly keep serving counts taken under the old list.
    protected static string BuildSignature()
    {
        BattleRoyaleConfig config = BattleRoyaleConfig.GetConfig();
        BattleRoyalePOIsData settings;
        string joined = "";
        int i;

        if (config)
            settings = config.GetPOIsData();

        if (settings && settings.poi_scan_exclude)
        {
            for (i = 0; i < settings.poi_scan_exclude.Count(); i++)
            {
                joined = joined + settings.poi_scan_exclude.Get(i) + ",";
            }
        }

        return "v" + CACHE_VERSION + "|cell" + BR_ZONE_CENSUS_CELL_M + "|ex:" + joined;
    }

    protected static bool LoadCache(string world_name, string signature)
    {
        BattleRoyaleScanCacheFile cache = BattleRoyaleScanCache.Get(world_name);
        int i;
        float value;

        if (!cache)
            return false;
        if (cache.census_version != CACHE_VERSION)
            return false;
        if (cache.census_signature != signature)
            return false;
        if (!cache.census_positions || cache.census_positions.Count() < 2)
            return false;

        //--- Copied out rather than aliased, and pairwise: an odd length would leave CountInCircle
        //--- reading half a position.
        for (i = 0; i + 1 < cache.census_positions.Count(); i = i + 2)
        {
            value = cache.census_positions.Get(i);
            s_Positions.Insert(value);

            value = cache.census_positions.Get(i + 1);
            s_Positions.Insert(value);
        }

        return true;
    }

    protected static void SaveCache(string world_name, string signature)
    {
        BattleRoyaleScanCacheFile cache = BattleRoyaleScanCache.Get(world_name);
        int i;
        float value;

        if (!cache)
            return;

        cache.world_name = world_name;
        cache.census_version = CACHE_VERSION;
        cache.census_signature = signature;
        cache.census_count = GetBuildingCount();
        cache.census_positions = new array<float>;

        for (i = 0; i < s_Positions.Count(); i++)
        {
            value = s_Positions.Get(i);
            cache.census_positions.Insert(value);
        }

        BattleRoyaleScanCache.Save();
        BattleRoyaleUtils.Info("[BuildingCensus] wrote " + cache.census_count + " buildings to scan_cache.json - later boots will not rescan.");
    }
}
#endif
