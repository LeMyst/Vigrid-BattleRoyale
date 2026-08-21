#ifdef SERVER

//--- One cached POI. Stored flat as [x, z, x, z, ...] rather than as an array<vector>: it halves the
//--- file, it matches the convention BattleRoyaleOverridePOIPosition already uses, and the height is
//--- not worth persisting because every consumer resolves it with GetGame().SurfaceY() at the moment
//--- of use anyway - exactly as GetOverrodePosition does.
class BattleRoyalePOICacheEntry
{
    string poi_name;
    float anchor_x;
    float anchor_z;
    float extent;
    int building_count;
    ref array<float> buildings;

    void BattleRoyalePOICacheEntry()
    {
        buildings = new array<float>;
    }
}

class BattleRoyalePOICacheFile
{
    int cache_version;
    string world_name;

    //--- Fingerprint of the scan PARAMETERS. Without it, editing poi_scan_radius_m would silently
    //--- keep serving results computed at the old radius - the cache would be a trap rather than an
    //--- optimisation, and the symptom (a setting that appears to do nothing) is the hardest kind to
    //--- attribute.
    string signature;

    ref array<ref BattleRoyalePOICacheEntry> entries;

    void BattleRoyalePOICacheFile()
    {
        entries = new array<ref BattleRoyalePOICacheEntry>;
    }
}

//--- Resolves a CfgWorlds "Names" POI to where the town ACTUALLY is.
//---
//--- WHY THIS EXISTS. A Names entry carries only name, position[] and type - no radius, no extent -
//--- and that position is a map-LABEL anchor placed to keep the text off the buildings. Measured on
//--- ChernarusPlus 2026-08-19: Settlement_Chernogorsk, the largest coastal city on the map, has TWO
//--- buildings within 100 m of its label (a news stand and a car wreck); the city proper does not
//--- appear until 200 m, reaching 173 buildings by 300 m. Prigorodki has four within 100 m.
//---
//--- Two consumers were both keyed on that label, and both are wrong in the same way: the spawn draw
//--- in 4_BattleRoyalePrepare (which centre-biases toward it with Math.Pow(rand, 2), so the offset is
//--- targeted rather than merely tolerated), and BattleRoyaleZone.PickSeedCenter, which with
//--- end_in_villages on centres the FINAL CIRCLE on the same empty field.
//---
//--- MEASURED PROPERTIES OF THE UNDERLYING QUERY (2026-08-19, ChernarusPlus, dedicated server):
//---   - Terrain-placed statics DO come back from GetObjectsAtPosition at arbitrary range, 4 km from
//---     any player. This was not established by reading vanilla, which has no call site doing it.
//---   - The object tree IS fully populated at MissionServer.OnInit: an OnInit pass and a lobby pass
//---     returned BIT-IDENTICAL counts at all nine samples. That is what lets the zone side, which
//---     generates its circles during Init(), consume this directly instead of a previous boot's cache.
//---   - Cost is FIRST-TOUCH PER REGION, not per query: 6-87 ms cold and 0 ms warm for the identical
//---     query. Extrapolated across ~306 POIs that is ~12 s of boot, on a ~76 s boot, paid every match
//---     because the server process restarts between them. THAT is why the cache below is not
//---     optional.
class BattleRoyalePOIResolver
{
    //--- Bump when the ALGORITHM changes in a way that invalidates stored results. Parameter changes
    //--- are caught by the signature instead, so this only moves for code changes.
    //--- v2: FillSample restricted to buildings inside the derived extent. A v1 cache holds fringe
    //--- outliers as spawn anchors, and nothing in the parameter signature would have caught that -
    //--- no SETTING changed, only the algorithm. This is what that distinction is for.
    static const int BR_POI_CACHE_VERSION = 2;

    //--- Distance weighting of the centroid. w = 1 - FALLOFF * (d/R)^2, so a building at the centre
    //--- counts 1.0 and one at the rim counts 0.25.
    //---
    //--- The falloff is what lets the scan radius be generous. A wide circle can catch a neighbouring
    //--- cluster - an adjacent farm, a satellite hamlet - and the naive fix is to shrink the radius,
    //--- which is wrong twice over: the derived extent can never exceed the scan radius, so shrinking
    //--- permanently mis-describes every large town; and mean-shift already converges on the densest
    //--- mass, which is the town. The weight only makes that convergence faster and steadier.
    //---
    //--- It deliberately does NOT reach zero at the rim. A kernel that does makes the whole result
    //--- sensitive to the exact radius, which is a setting, and turns a tuning change into a cliff.
    static const float BR_POI_WEIGHT_FALLOFF = 0.75;

    static ref map<string, ref BattleRoyalePOICacheEntry> s_Resolved;
    static bool s_Ready = false;

    //--- Call once, from BattleRoyaleServer.Init(). Loads the cache when it is valid for this world
    //--- and this parameter set, otherwise scans every POI and writes a fresh one.
    static void Init()
    {
        if (s_Ready)
            return;

        s_Ready = true;
        s_Resolved = new map<string, ref BattleRoyalePOICacheEntry>;

        BattleRoyalePOIsData settings = BattleRoyaleConfig.GetConfig().GetPOIsData();
        if (!settings || !settings.resolve_poi_from_buildings)
        {
            BattleRoyaleUtils.Info("[POIResolver] resolve_poi_from_buildings is off - every POI keeps its CfgWorlds label.");
            return;
        }

        string world_name = "";
        GetGame().GetWorldName(world_name);

        string signature = BuildSignature(settings);

        //--- The EFFECTIVE configuration, read back off the live object rather than trusted from the
        //--- JSON on disk. This exists because the two are not always the same: a mission override
        //--- pass silently empties any `ref array` its file does not mention (see the note in
        //--- BattleRoyalePOIsData.LoadMission), and the symptom of that is a guard which is simply
        //--- never applied - no error, no warning, and a profile JSON that looks perfectly correct.
        //--- An empty `types=` here is the tell, and it costs one line a boot to have it.
        string cfgline = "[POIResolver] radius " + settings.poi_scan_radius_m.ToString();
        cfgline = cfgline + " m, iterations " + settings.poi_scan_iterations.ToString();
        cfgline = cfgline + ", max shift " + settings.poi_max_shift_m.ToString();
        cfgline = cfgline + " m, types [" + JoinTypes(settings) + "]";
        cfgline = cfgline + ", excludes [" + JoinExcludes(settings) + "]";
        BattleRoyaleUtils.Info(cfgline);

        if (LoadCache(world_name, signature))
        {
            BattleRoyaleUtils.Info("[POIResolver] cache hit for " + world_name + " - " + s_Resolved.Count() + " POIs, no scan needed.");
            RunSelfTest(settings);
            return;
        }

        ScanAll(settings, world_name, signature);
        RunSelfTest(settings);
    }

    //--- True when this POI resolved to something we trust. A POI that failed poi_min_buildings is
    //--- deliberately absent, so every accessor degrades to the caller's own fallback.
    static bool IsResolved(string poi_name)
    {
        if (!s_Resolved)
            return false;

        return s_Resolved.Contains(poi_name);
    }

    //--- The town centre, or `fallback` (the caller's CfgWorlds label) when unresolved. Y comes from
    //--- the terrain, never from the cache.
    static vector GetAnchor(string poi_name, vector fallback)
    {
        if (!IsResolved(poi_name))
            return fallback;

        BattleRoyalePOICacheEntry entry = s_Resolved.Get(poi_name);
        if (!entry)
            return fallback;

        vector anchor = "0 0 0";
        anchor[0] = entry.anchor_x;
        anchor[2] = entry.anchor_z;
        anchor[1] = GetGame().SurfaceY(anchor[0], anchor[2]);
        return anchor;
    }

    //--- The town's radius, or `fallback` when unresolved. This is what retires the hardcoded
    //--- 500 / 300 / 100 pads in 4_BattleRoyalePrepare.
    static float GetExtent(string poi_name, float fallback)
    {
        if (!IsResolved(poi_name))
            return fallback;

        BattleRoyalePOICacheEntry entry = s_Resolved.Get(poi_name);
        if (!entry)
            return fallback;

        //--- A stored extent of 0 means "no opinion", not "zero metres wide". An admin override is
        //--- recorded exactly that way (StoreOverride) because moving a POI says where it is, not how
        //--- big it is - and returning the 0 literally would collapse the caller's spawn radius to a
        //--- point, dropping every player of that town on one square metre.
        if (entry.extent <= 0)
            return fallback;

        return entry.extent;
    }

    /**
     *  Lootable buildings around `center`, for anyone who needs a building census rather than a POI.
     *
     *  A thin public door onto CollectBuildings, so the two rules that make the count meaningful stay
     *  in ONE place: Building.Cast rather than IsBuilding() (see CollectBuildings for why Well and
     *  FuelStation would otherwise be dropped), and the poi_scan_exclude list, which removes wrecks,
     *  fences, gates and mobile toilets - town furniture that is a Building and holds no loot.
     *
     *  Added for BattleRoyaleZone's derived ladder (#284): POI COUNT DOES NOT MEASURE LOOT. Sampled
     *  over 400 random circles per radius against the Central Economy's own loot points - lootmax per
     *  building type from mapgroupproto.xml times every instance in mapgrouppos.xml - POI count
     *  correlates 0.15-0.29, and filtering it down to settlement types does not rescue it (0.01-0.42).
     *  The count of buildings inside the circle correlates 0.993-0.995.
     */
    static void CollectBuildingsAt(vector center, float radius, out array<vector> found)
    {
        BattleRoyaleConfig config = BattleRoyaleConfig.GetConfig();
        if (!config)
            return;

        BattleRoyalePOIsData settings = config.GetPOIsData();
        if (!settings)
            return;

        CollectBuildings(settings, center, found, radius);
    }

    //--- A random building position in this town.
    //---
    //--- This, rather than the anchor, is what the spawn side should prefer: a centroid alone puts
    //--- players in the gap of an L-shaped or valley town, whereas "a few metres from a real building"
    //--- is correct for any town shape and is what a battle-royale drop is supposed to feel like.
    static bool GetRandomBuildingPos(string poi_name, out vector pos)
    {
        pos = "0 0 0";

        if (!IsResolved(poi_name))
            return false;

        BattleRoyalePOICacheEntry entry = s_Resolved.Get(poi_name);
        if (!entry || !entry.buildings)
            return false;

        int pairs = entry.buildings.Count() / 2;
        if (pairs <= 0)
            return false;

        int pick = Math.RandomInt(0, pairs);

        //--- Each element is read into a local on its own line, before any call. An array read sharing
        //--- an expression with a call has been measured in this codebase to return an element of a
        //--- DIFFERENT array - see the aliasing note in BattleRoyaleZone.ComputeAllowRadii.
        float bx = entry.buildings.Get(pick * 2);
        float bz = entry.buildings.Get((pick * 2) + 1);

        pos[0] = bx;
        pos[2] = bz;
        pos[1] = GetGame().SurfaceY(bx, bz);
        return true;
    }

    //--- A spawn candidate: a random building of this town, offset a few metres in a random
    //--- direction. Lives here rather than in the caller so the acceptance self-test in
    //--- BattleRoyaleDebug measures exactly what BattleRoyalePrepare will use, instead of a
    //--- reimplementation of it that could drift.
    static bool GetSpawnCandidate(string poi_name, out vector pos)
    {
        pos = "0 0 0";

        vector building_pos;
        if (!GetRandomBuildingPos(poi_name, building_pos))
            return false;

        float spread = BR_SPAWN_BUILDING_OFFSET_MAX_M - BR_SPAWN_BUILDING_OFFSET_MIN_M;
        float away = BR_SPAWN_BUILDING_OFFSET_MIN_M + Math.RandomFloat(0, spread);
        float bearing = Math.RandomFloat(0, 360) * Math.DEG2RAD;

        float bx = building_pos[0] + (away * Math.Cos(bearing));
        float bz = building_pos[2] + (away * Math.Sin(bearing));

        pos[0] = bx;
        pos[2] = bz;
        pos[1] = GetGame().SurfaceY(bx, bz);
        return true;
    }

    //--- Every POI that resolved, for callers that need to walk them (the acceptance self-test).
    static void GetResolvedNames(out array<string> names)
    {
        if (!s_Resolved)
            return;

        int i;
        for (i = 0; i < s_Resolved.Count(); i++)
        {
            string key = s_Resolved.GetKey(i);
            names.Insert(key);
        }
    }

    // ---------------------------------------------------------------------------------------------
    // Scanning
    // ---------------------------------------------------------------------------------------------

    protected static void ScanAll(BattleRoyalePOIsData settings, string world_name, string signature)
    {
        string cfg = "CfgWorlds " + world_name + " Names";
        int total = GetGame().ConfigGetChildrenCount(cfg);

        float t0 = GetGame().GetTickTime();
        int resolved = 0;
        int skipped_type = 0;
        int rejected_sparse = 0;
        int rejected_far = 0;
        int i;

        for (i = 0; i < total; i++)
        {
            string poi_name;
            GetGame().ConfigGetChildName(cfg, i, poi_name);

            vector label;
            if (!ReadLabel(cfg, poi_name, label))
                continue;

            //--- An admin override is absolute and is never rescanned. Recording it as a resolved
            //--- entry (with no building list) means every consumer can read one accessor instead of
            //--- checking two sources in the right order - which is how the two existing consumers
            //--- came to apply the override at different points in the first place.
            vector admin = settings.GetOverrodePosition(poi_name);
            if (admin != "0 0 0")
            {
                StoreOverride(poi_name, admin);
                resolved++;
                continue;
            }

            //--- Below the admin override on purpose: an override is an explicit instruction and
            //--- applies whatever the POI's type is. Above the scan, so a non-settlement is never
            //--- scanned at all - which is most of the cold-boot cost saved as well as the fix.
            string poi_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, poi_name));
            if (!IsResolvableType(settings, poi_type))
            {
                skipped_type++;
                continue;
            }

            BattleRoyalePOICacheEntry entry;
            string reject = "";

            if (ScanPOI(settings, poi_name, label, entry, reject))
            {
                s_Resolved.Set(poi_name, entry);
                resolved++;
                continue;
            }

            //--- Counted per reason, never as one total. "Sparse" and "moved too far" are different
            //--- faults with different fixes (poi_min_buildings vs poi_max_shift_m or the type list),
            //--- and a single rejected count cannot tell an operator which knob to reach for.
            if (reject == "far")
                rejected_far++;
            else
                rejected_sparse++;
        }

        float elapsed_ms = (GetGame().GetTickTime() - t0) * 1000.0;

        //--- Built in steps: a single expression has a complexity ceiling around ten concatenated
        //--- terms and is rejected outright with "Formula too complex", which packs fine and only
        //--- surfaces when the module loads.
        string line = "[POIResolver] scanned ";
        line = line + total.ToString() + " POIs in " + world_name;
        line = line + " - resolved " + resolved.ToString();
        line = line + ", skipped by type " + skipped_type.ToString();
        line = line + ", too sparse " + rejected_sparse.ToString();
        line = line + ", moved too far " + rejected_far.ToString();
        line = line + " (" + elapsed_ms.ToString() + " ms)";
        BattleRoyaleUtils.Info(line);

        SaveCache(world_name, signature);
    }

    //--- Mean shift. The first pass is centred on the label, which is off-town by construction, so a
    //--- single pass would weight half its sample on empty ground; each later pass re-centres on the
    //--- running centroid and converges on the densest mass.
    protected static bool ScanPOI(BattleRoyalePOIsData settings, string poi_name, vector label, out BattleRoyalePOICacheEntry entry, out string reject)
    {
        entry = NULL;
        reject = "sparse";

        vector center = label;
        ref array<vector> found = new array<vector>;
        float last_move = 0;
        int pass;

        for (pass = 0; pass < settings.poi_scan_iterations; pass++)
        {
            found.Clear();
            CollectBuildings(settings, center, found);

            if (found.Count() == 0)
                return false;

            vector next = WeightedCentroid(settings, center, found);

            last_move = vector.Distance(next, center);
            center = next;

            if (last_move < BR_POI_RESOLVE_CONVERGE_M)
                break;
        }

        //--- Re-collect about the settled centre so the stored buildings and the extent are measured
        //--- from where the anchor actually ended up - but ONLY when the last step was big enough to
        //--- matter. On the converged path the sample in hand was taken within
        //--- BR_POI_RESOLVE_CONVERGE_M of the final centre, which is what convergence means, so
        //--- re-collecting would buy nothing and this is the single most expensive call in the file.
        if (last_move >= BR_POI_RESOLVE_CONVERGE_M)
        {
            found.Clear();
            CollectBuildings(settings, center, found);
        }

        if (found.Count() < settings.poi_min_buildings)
        {
            //--- A lone deer stand is not a town. Moving a POI onto one is worse than leaving it.
            return false;
        }

        //--- The map-agnostic guard. A scan that lands this far away has not found THIS POI's town,
        //--- it has found a different place - measured on ChernarusPlus, Hill_Kopyto resolved 510 m
        //--- onto an unrelated coastal town, which would then share a seed with that town's own POI
        //--- and quietly halve the final-circle spread. Legitimate settlement corrections are well
        //--- inside this: the largest measured is Chernogorsk at 291 m.
        float sdx = center[0] - label[0];
        float sdz = center[2] - label[2];
        float shift = Math.Sqrt((sdx * sdx) + (sdz * sdz));

        if (shift > settings.poi_max_shift_m)
        {
            reject = "far";
            return false;
        }

        entry = new BattleRoyalePOICacheEntry();
        entry.poi_name = poi_name;
        entry.anchor_x = center[0];
        entry.anchor_z = center[2];
        entry.building_count = found.Count();
        entry.extent = ComputeExtent(settings, center, found);

        FillSample(entry, center, found);
        return true;
    }

    //--- `radius` defaults to the configured POI scan radius, which is what every resolver call site
    //--- wants; CollectBuildingsAt passes its own so a census can tile the map at its own cell size.
    protected static void CollectBuildings(BattleRoyalePOIsData settings, vector center, out array<vector> found, float radius = -1)
    {
        ref array<Object> objects = new array<Object>;
        ref array<CargoBase> cargos = new array<CargoBase>;

        if (radius <= 0)
            radius = settings.poi_scan_radius_m;

        GetGame().GetObjectsAtPosition(center, radius, objects, cargos);

        int i;
        for (i = 0; i < objects.Count(); i++)
        {
            Object obj = objects.Get(i);
            if (!obj)
                continue;

            //--- Building.Cast, never obj.IsBuilding(): Well and FuelStation both override
            //--- IsBuilding() to return false (P:\scripts\4_world\entities\building\well.c:3 and
            //--- fuelstation.c:3), and both are genuine town furniture worth counting. Trees, bushes
            //--- and rocks are not Building at all, so they filter out for free.
            Building building = Building.Cast(obj);
            if (!building)
                continue;

            if (IsExcluded(settings, obj.GetType()))
                continue;

            found.Insert(obj.GetPosition());
        }
    }

    //--- An EMPTY list means "no type filter", not "match nothing" - see the field comment in
    //--- BattleRoyalePOIsData. The benign reading is deliberate: on a map whose type vocabulary
    //--- differs from Chernarus's, the worst case is the unfiltered behaviour that poi_max_shift_m
    //--- still guards, rather than a silent total outage.
    protected static bool IsResolvableType(BattleRoyalePOIsData settings, string poi_type)
    {
        if (!settings.poi_resolve_types || settings.poi_resolve_types.Count() == 0)
            return true;

        int i;
        for (i = 0; i < settings.poi_resolve_types.Count(); i++)
        {
            string wanted = settings.poi_resolve_types.Get(i);
            if (wanted == poi_type)
                return true;
        }

        return false;
    }

    protected static bool IsExcluded(BattleRoyalePOIsData settings, string class_name)
    {
        if (!settings.poi_scan_exclude || class_name == "")
            return false;

        int i;
        for (i = 0; i < settings.poi_scan_exclude.Count(); i++)
        {
            string fragment = settings.poi_scan_exclude.Get(i);
            if (fragment == "")
                continue;

            if (class_name.Contains(fragment))
                return true;
        }

        return false;
    }

    protected static vector WeightedCentroid(BattleRoyalePOIsData settings, vector center, array<vector> found)
    {
        float sum_x = 0;
        float sum_z = 0;
        float sum_w = 0;
        int i;

        for (i = 0; i < found.Count(); i++)
        {
            vector p = found.Get(i);

            float dx = p[0] - center[0];
            float dz = p[2] - center[2];
            float dist = Math.Sqrt((dx * dx) + (dz * dz));

            float w = WeightFor(settings, dist);

            sum_x = sum_x + (p[0] * w);
            sum_z = sum_z + (p[2] * w);
            sum_w = sum_w + w;
        }

        if (sum_w <= 0)
            return center;

        vector result = "0 0 0";
        result[0] = sum_x / sum_w;
        result[2] = sum_z / sum_w;
        return result;
    }

    protected static float WeightFor(BattleRoyalePOIsData settings, float dist)
    {
        if (settings.poi_scan_radius_m <= 0)
            return 1.0;

        float ratio = dist / settings.poi_scan_radius_m;
        if (ratio > 1.0)
            ratio = 1.0;

        float w = 1.0 - (BR_POI_WEIGHT_FALLOFF * ratio * ratio);
        if (w < 0.01)
            w = 0.01;

        return w;
    }

    //--- The distance covering poi_extent_percentile of the buildings - NOT the maximum, or a single
    //--- outlying barn triples the pad for the whole town.
    protected static float ComputeExtent(BattleRoyalePOIsData settings, vector center, array<vector> found)
    {
        ref array<float> distances = new array<float>;
        int i;

        for (i = 0; i < found.Count(); i++)
        {
            vector p = found.Get(i);

            float dx = p[0] - center[0];
            float dz = p[2] - center[2];
            distances.Insert(Math.Sqrt((dx * dx) + (dz * dz)));
        }

        distances.Sort();

        int idx = Math.Round(settings.poi_extent_percentile * (distances.Count() - 1));
        if (idx < 0)
            idx = 0;
        if (idx >= distances.Count())
            idx = distances.Count() - 1;

        float extent = distances.Get(idx);

        if (extent < settings.poi_extent_min_m)
            extent = settings.poi_extent_min_m;
        if (extent > settings.poi_extent_max_m)
            extent = settings.poi_extent_max_m;

        return extent;
    }

    //--- Keeps at most BR_POI_SAMPLE_CAP buildings, evenly spread through the collected list rather
    //--- than the first N. The engine returns objects in its own spatial order, so taking a prefix
    //--- would systematically favour one part of the town.
    //---
    //--- ⚠️ ONLY BUILDINGS INSIDE THE DERIVED EXTENT ARE KEPT, and that filter is the difference
    //--- between "spawned in the village" and "spawned next to a shed in a field".
    //---
    //--- The scan reaches poi_scan_radius_m (350) so that a large town's extent can be measured at
    //--- all, but the extent itself is the 80th percentile - so the outer band of every collection is
    //--- fringe: an isolated barn, a lone outbuilding, the corner of the next hamlet. Sampling evenly
    //--- across the whole collection gives those the same weight as a house on the main street.
    //---
    //--- Measured live: a player dropped at Vybor landed 16.5 m from Land_Shed_M3, a shed over 300 m
    //--- from the town centre and outside the 203 m extent - technically "next to a building" and
    //--- exactly the wrong experience. Both towns sampled that day spanned 26-330 m from their
    //--- anchor, with 5 of 32 entries beyond the extent.
    protected static void FillSample(BattleRoyalePOICacheEntry entry, vector center, array<vector> found)
    {
        ref array<vector> core = new array<vector>;
        int i;

        for (i = 0; i < found.Count(); i++)
        {
            vector p = found.Get(i);

            float dx = p[0] - center[0];
            float dz = p[2] - center[2];

            if (Math.Sqrt((dx * dx) + (dz * dz)) <= entry.extent)
                core.Insert(p);
        }

        //--- Nothing inside the extent should be impossible - the extent is a percentile OF these
        //--- same distances, so at least 80% of them qualify by construction - but a degenerate town
        //--- clamped by poi_extent_min_m could in principle empty it, and a spawn table with no
        //--- entries silently disables the building path for that town.
        if (core.Count() == 0)
            core = found;

        int step = core.Count() / BR_POI_SAMPLE_CAP;
        if (step < 1)
            step = 1;

        int taken = 0;

        for (i = 0; i < core.Count() && taken < BR_POI_SAMPLE_CAP; i += step)
        {
            vector q = core.Get(i);

            entry.buildings.Insert(q[0]);
            entry.buildings.Insert(q[2]);
            taken++;
        }
    }

    protected static void StoreOverride(string poi_name, vector admin)
    {
        BattleRoyalePOICacheEntry entry = new BattleRoyalePOICacheEntry();
        entry.poi_name = poi_name;
        entry.anchor_x = admin[0];
        entry.anchor_z = admin[2];
        entry.building_count = 0;

        //--- Zero, so GetExtent falls through to the caller's own value. An admin who moves a POI is
        //--- saying where it is, not how big it is.
        entry.extent = 0;

        s_Resolved.Set(poi_name, entry);
    }

    protected static bool ReadLabel(string cfg, string poi_name, out vector label)
    {
        label = "0 0 0";

        TFloatArray raw = {};
        GetGame().ConfigGetFloatArray(string.Format("%1 %2 position", cfg, poi_name), raw);

        if (raw.Count() < 2)
            return false;

        //--- position[] is a 2-element XZ pair with no height on every world checked, so Y has to come
        //--- from the terrain.
        label[0] = raw.Get(0);
        label[2] = raw.Get(1);
        label[1] = GetGame().SurfaceY(label[0], label[2]);
        return true;
    }

    // ---------------------------------------------------------------------------------------------
    // Cache
    // ---------------------------------------------------------------------------------------------

    //--- MISSION folder, not profile - and that is the right home rather than a preference.
    //---
    //--- This cache is per-MAP data (it is a description of one world's towns), and the mission is the
    //--- thing in this stack that is already per-map. Keeping them together means a server that runs
    //--- several maps gets one cache each for free, they travel with the mission if it is copied to
    //--- another host, and deleting a mission takes its derived data with it instead of leaving an
    //--- orphan in the profile.
    //---
    //--- Note this makes it the ONE file the mod writes under $mission:. Every settings class writes
    //--- only to $profile: on purpose, because those are the admin's hand-edited inputs and a mission
    //--- copy would silently shadow them - but this is generated output that nobody edits, so that
    //--- reasoning does not apply. The world_name check in LoadCache stays anyway: it costs nothing
    //--- and it still catches one mission folder being pointed at a different terrain.
    protected static string GetCachePath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "poi_cache.json";
    }

    //--- Every parameter that changes the result. Anything added to the scan MUST be added here too,
    //--- or that setting becomes silently inert on any server that already has a cache file.
    protected static string BuildSignature(BattleRoyalePOIsData settings)
    {
        string sig = "r" + settings.poi_scan_radius_m.ToString();
        sig = sig + "|i" + settings.poi_scan_iterations.ToString();
        sig = sig + "|m" + settings.poi_min_buildings.ToString();
        sig = sig + "|p" + settings.poi_extent_percentile.ToString();
        sig = sig + "|lo" + settings.poi_extent_min_m.ToString();
        sig = sig + "|hi" + settings.poi_extent_max_m.ToString();
        sig = sig + "|x" + JoinExcludes(settings);
        sig = sig + "|s" + settings.poi_max_shift_m.ToString();
        sig = sig + "|t" + JoinTypes(settings);

        //--- An admin override is applied during the scan and baked into the cache, so the cache has
        //--- to be rebuilt when that list changes.
        sig = sig + "|o" + CountOverrides(settings).ToString();
        return sig;
    }

    protected static string JoinExcludes(BattleRoyalePOIsData settings)
    {
        if (!settings.poi_scan_exclude)
            return "";

        string joined = "";
        int i;

        for (i = 0; i < settings.poi_scan_exclude.Count(); i++)
        {
            string fragment = settings.poi_scan_exclude.Get(i);
            joined = joined + fragment + ",";
        }

        return joined;
    }

    protected static string JoinTypes(BattleRoyalePOIsData settings)
    {
        if (!settings.poi_resolve_types)
            return "";

        string joined = "";
        int i;

        for (i = 0; i < settings.poi_resolve_types.Count(); i++)
        {
            string wanted = settings.poi_resolve_types.Get(i);
            joined = joined + wanted + ",";
        }

        return joined;
    }

    protected static int CountOverrides(BattleRoyalePOIsData settings)
    {
        if (!settings.override_poi_positions)
            return 0;

        return settings.override_poi_positions.Count();
    }

    protected static bool LoadCache(string world_name, string signature)
    {
        if (!FileExist(GetCachePath()))
            return false;

        string error_message;
        ref BattleRoyalePOICacheFile file = new BattleRoyalePOICacheFile();

        if (!JsonFileLoader<BattleRoyalePOICacheFile>.LoadFile(GetCachePath(), file, error_message))
        {
            //--- Warn, never Error: BattleRoyaleUtils.Error routes to Error2(), which raises a VM
            //--- exception and would take server init down over a corrupt cache file that we are
            //--- perfectly able to rebuild.
            BattleRoyaleUtils.Warn("[POIResolver] poi_cache.json could not be read (" + error_message + ") - rescanning.");
            return false;
        }

        if (file.cache_version != BR_POI_CACHE_VERSION)
        {
            BattleRoyaleUtils.Info("[POIResolver] cache is version " + file.cache_version + ", expected " + BR_POI_CACHE_VERSION + " - rescanning.");
            return false;
        }

        //--- A server that switches map must not read the previous map's towns. Nothing else keys on
        //--- the world, so without this the cache is actively dangerous rather than merely stale.
        if (file.world_name != world_name)
        {
            BattleRoyaleUtils.Info("[POIResolver] cache was built for " + file.world_name + ", this is " + world_name + " - rescanning.");
            return false;
        }

        if (file.signature != signature)
        {
            BattleRoyaleUtils.Info("[POIResolver] scan settings changed since the cache was written - rescanning.");
            return false;
        }

        if (!file.entries || file.entries.Count() == 0)
            return false;

        int i;
        for (i = 0; i < file.entries.Count(); i++)
        {
            BattleRoyalePOICacheEntry entry = file.entries.Get(i);
            if (!entry || entry.poi_name == "")
                continue;

            //--- A `ref array` field does NOT get its initialiser back after deserialization, so an
            //--- entry can legitimately load with a null list. Repair it rather than letting every
            //--- later Count() call dereference null - the anchor and extent are still good.
            if (!entry.buildings)
                entry.buildings = new array<float>;

            s_Resolved.Set(entry.poi_name, entry);
        }

        return s_Resolved.Count() > 0;
    }

    protected static void SaveCache(string world_name, string signature)
    {
        ref BattleRoyalePOICacheFile file = new BattleRoyalePOICacheFile();
        file.cache_version = BR_POI_CACHE_VERSION;
        file.world_name = world_name;
        file.signature = signature;

        int i;
        for (i = 0; i < s_Resolved.Count(); i++)
        {
            BattleRoyalePOICacheEntry entry = s_Resolved.GetElement(i);
            if (entry)
                file.entries.Insert(entry);
        }

        //--- $mission:Vigrid-BattleRoyale\ is optional for the settings classes - they only ever READ
        //--- from it - so on most servers it does not exist at all and the save would fail on a
        //--- missing directory. MakeDirectory is a no-op when it is already there.
        MakeDirectory(BATTLEROYALE_SETTINGS_MISSION_FOLDER);

        string error_message;
        if (!JsonFileLoader<BattleRoyalePOICacheFile>.SaveFile(GetCachePath(), file, error_message))
        {
            //--- Not fatal. The scan already ran and this boot is fully resolved; only the NEXT boot
            //--- pays for the failure, by scanning again.
            BattleRoyaleUtils.Warn("[POIResolver] could not write poi_cache.json (" + error_message + ") - the next boot will rescan.");
            return;
        }

        BattleRoyaleUtils.Info("[POIResolver] wrote " + file.entries.Count() + " POIs to poi_cache.json");
    }

    // ---------------------------------------------------------------------------------------------
    // Self test
    // ---------------------------------------------------------------------------------------------

    //--- The acceptance gate, and the reason it is a SETTING rather than a diag entry is that it has
    //--- to run on a headless dedicated server - the same reasoning as zone_selftest_runs.
    //---
    //--- THE COLUMN THAT MATTERS IS shift. An all-zero shift column means the scan returned nothing
    //--- and the whole feature is silently inert, which is the exact failure mode this codebase has
    //--- already shipped once (BR_ZONE_OFFSET_MIN_DISTANCE sat at a threshold nothing could reach, so
    //--- its array was all zeros for months). A count is not enough on its own: a scan that resolved
    //--- every POI to its own label would report a perfect resolved count and change nothing.
    protected static void RunSelfTest(BattleRoyalePOIsData settings)
    {
        if (settings.poi_resolve_selftest <= 0)
            return;

        string world_name = "";
        GetGame().GetWorldName(world_name);
        string cfg = "CfgWorlds " + world_name + " Names";

        int total = GetGame().ConfigGetChildrenCount(cfg);
        int shown = 0;
        int moved_count = 0;
        float shift_sum = 0;
        int i;

        BattleRoyaleUtils.Info("[POIResolver] self test - poi / type / label / resolved / shift m / buildings / extent m");

        for (i = 0; i < total; i++)
        {
            string poi_name;
            GetGame().ConfigGetChildName(cfg, i, poi_name);

            vector label;
            if (!ReadLabel(cfg, poi_name, label))
                continue;

            if (!IsResolved(poi_name))
                continue;

            vector anchor = GetAnchor(poi_name, label);

            //--- Horizontal only. Y is terrain height at two different points, so including it would
            //--- add a hillside's rise to the number and make a shift look larger than it is.
            float dx = anchor[0] - label[0];
            float dz = anchor[2] - label[2];
            float shift = Math.Sqrt((dx * dx) + (dz * dz));

            shift_sum = shift_sum + shift;
            if (shift >= 1.0)
                moved_count++;

            if (shown < settings.poi_resolve_selftest)
            {
                string poi_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, poi_name));

                string row = "[POIResolver]   " + poi_name;
                row = row + " (" + poi_type + ")";
                row = row + " label=" + label[0].ToString() + "," + label[2].ToString();
                row = row + " -> " + anchor[0].ToString() + "," + anchor[2].ToString();
                row = row + " shift=" + shift.ToString();
                BattleRoyaleUtils.Info(row);

                string row2 = "[POIResolver]     buildings=";
                row2 = row2 + GetBuildingCount(poi_name).ToString();
                row2 = row2 + " extent=" + GetExtent(poi_name, 0).ToString();
                BattleRoyaleUtils.Info(row2);

                shown++;
            }
        }

        int resolved = s_Resolved.Count();
        if (resolved == 0)
        {
            BattleRoyaleUtils.Warn("[POIResolver] SELF TEST: nothing resolved at all. The scan is inert - check that GetObjectsAtPosition returns statics on this map.");
            return;
        }

        float mean_shift = shift_sum / resolved;

        string summary = "[POIResolver] SELF TEST: resolved ";
        summary = summary + resolved.ToString() + " of " + total.ToString();
        summary = summary + ", moved " + moved_count.ToString();
        summary = summary + ", mean shift " + mean_shift.ToString() + " m";
        BattleRoyaleUtils.Info(summary);

        //--- The dead-feature alarm. Resolving everything and moving nothing is indistinguishable from
        //--- the feature being switched off, and reads as success in every other column.
        if (moved_count == 0)
            BattleRoyaleUtils.Warn("[POIResolver] SELF TEST: every POI resolved to its own label. That is the signature of a scan that found nothing usable - treat it as a failure, not a pass.");
    }

    protected static int GetBuildingCount(string poi_name)
    {
        if (!IsResolved(poi_name))
            return 0;

        BattleRoyalePOICacheEntry entry = s_Resolved.Get(poi_name);
        if (!entry)
            return 0;

        return entry.building_count;
    }
}

#endif
