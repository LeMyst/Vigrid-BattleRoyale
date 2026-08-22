#ifdef SERVER
/**
 *  The one file both world scans are cached in.
 *
 *  Two things are expensive enough at boot to be worth keeping between runs, and both are derived
 *  from the same underlying query (GetObjectsAtPosition over terrain statics):
 *
 *    POI ANCHORS   - where each town actually is, ~12 s cold across 306 POIs
 *    BUILDING CENSUS - every lootable building on the map, ~40 s cold over 961 probes
 *
 *  They were two files. One is easier to reason about, easier to delete when an operator wants a
 *  clean rescan, and makes the "what did this server scan" question a single read.
 *
 *  ⚠️ EACH SECTION KEEPS ITS OWN SIGNATURE, and that is the whole design rather than a detail. The two
 *  are invalidated by completely different settings - the POI side by scan radius, iterations, types,
 *  max shift and the override list; the census side by cell size and the exclusion list - so a single
 *  shared signature would throw away a 40 s building scan every time somebody nudged poi_max_shift_m.
 *  A section whose signature no longer matches is rescanned ALONE; the other is kept.
 *
 *  ⚠️ THE CONTAINER IS HELD IN MEMORY AND SAVED WHOLE. Both consumers mutate the same instance, so
 *  whichever writes last still carries the other's section. Writing a section straight to disk without
 *  reading the file back first would silently truncate whatever the other one had just stored.
 */
class BattleRoyaleScanCacheFile
{
    //--- Format of the CONTAINER. The per-section versions below are what actually gate a rescan;
    //--- this only changes if the envelope itself is reshaped.
    int cache_version;

    //--- A server that switches map must not read the previous map's towns. Checked once here rather
    //--- than per section, because it invalidates everything equally.
    string world_name;

    int poi_version;
    string poi_signature;
    ref array<ref BattleRoyalePOICacheEntry> poi_entries;

    int census_version;
    string census_signature;
    int census_count;
    ref array<float> census_positions;

    void BattleRoyaleScanCacheFile()
    {
        poi_entries = new array<ref BattleRoyalePOICacheEntry>;
        census_positions = new array<float>;
    }
}

class BattleRoyaleScanCache
{
    static const int CONTAINER_VERSION = 1;

    static ref BattleRoyaleScanCacheFile s_File;
    static bool s_Loaded;

    static string GetPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "scan_cache.json";
    }

    //--- The pre-consolidation POI cache. Read once, so a server updating to this build does not throw
    //--- away a warm POI scan it already paid for; nothing ever writes it again.
    static string GetLegacyPOIPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "poi_cache.json";
    }

    /**
     *  The container, loaded on first ask. Never NULL: a missing, unreadable, wrong-version or
     *  wrong-map file yields an empty container, which both sections then read as "nothing cached".
     */
    static BattleRoyaleScanCacheFile Get(string world_name)
    {
        string error_message;
        BattleRoyaleScanCacheFile loaded;

        if (s_Loaded)
            return s_File;

        s_Loaded = true;
        s_File = new BattleRoyaleScanCacheFile();
        s_File.cache_version = CONTAINER_VERSION;
        s_File.world_name = world_name;

        if (!FileExist(GetPath()))
            return s_File;

        loaded = new BattleRoyaleScanCacheFile();
        if (!JsonFileLoader<BattleRoyaleScanCacheFile>.LoadFile(GetPath(), loaded, error_message))
        {
            //--- Warn, never Error: BattleRoyaleUtils.Error raises a VM exception and would take server
            //--- init down over a corrupt cache we are perfectly able to rebuild.
            BattleRoyaleUtils.Warn("[ScanCache] scan_cache.json could not be read (" + error_message + ") - both scans will run again.");
            return s_File;
        }

        if (loaded.cache_version != CONTAINER_VERSION)
        {
            BattleRoyaleUtils.Info("[ScanCache] container is version " + loaded.cache_version + ", expected " + CONTAINER_VERSION + " - both scans will run again.");
            return s_File;
        }

        if (loaded.world_name != world_name)
        {
            BattleRoyaleUtils.Info("[ScanCache] cache was built for " + loaded.world_name + ", this is " + world_name + " - both scans will run again.");
            return s_File;
        }

        s_File = loaded;
        return s_File;
    }

    static void Save()
    {
        string error_message;

        if (!s_File)
            return;

        //--- $mission:Vigrid-BattleRoyale\ is optional for the settings classes, which only ever READ
        //--- from it, so on most servers it does not exist and the save would fail on a missing
        //--- directory. MakeDirectory is a no-op when it is already there.
        MakeDirectory(BATTLEROYALE_SETTINGS_MISSION_FOLDER);

        if (!JsonFileLoader<BattleRoyaleScanCacheFile>.SaveFile(GetPath(), s_File, error_message))
        {
            //--- Not fatal. This boot is fully scanned; only the NEXT one pays, by scanning again.
            BattleRoyaleUtils.Warn("[ScanCache] could not write scan_cache.json (" + error_message + ") - the next boot will rescan.");
            return;
        }
    }
}
#endif
