#ifdef SERVER
class BattleRoyaleZone
{
    protected ref BattleRoyaleZone m_ParentZone;
    protected ref BattleRoyalePlayArea m_PlayArea;

    protected BattleRoyaleConfig m_Config;
    protected BattleRoyaleZoneData m_ZoneSettings;

    protected float f_ConstantShrink;
    protected int i_ShrinkType;
    protected int i_NumRounds;

    protected float f_Eulers;
    protected float f_Exponent;
    protected ref array<float> a_StaticSizes;
    protected ref array<int> a_StaticTimers;
    protected ref array<int> a_MinPlayers;

    //--- Derived ladder (#284). When on, GetZoneMinPlayers answers from s_PlayAreaMinPlayers - built
    //--- once from the PLACED circles - instead of from a_MinPlayers. a_MinPlayers stays the admin's
    //--- untouched input, exactly as a_StaticSizes does under radius flex.
    protected bool b_DeriveLadder;
    protected float f_MetresPerPlayer;
    protected int i_MinPlayersFloor;
    protected float f_LootDensityWeight;
    protected float f_LootFactorMin;
    protected float f_LootFactorMax;

    protected bool b_EndInVillages;

    //--- Tier-1 land requirement, from zone_settings so it can be tuned per map without a rebuild.
    protected float f_MinLandFraction;

    protected ref array<string> a_avoidType;
    protected ref array<string> a_avoidCity;

    protected int i_RoundDurationMinutes;

    protected ref array<vector> polygon_vertices;

    //--- Bounding box of polygon_vertices, computed once in Init() alongside it. The old code
    //--- recomputed this inside the generation function every time it ran.
    protected float f_PolyMinX;
    protected float f_PolyMaxX;
    protected float f_PolyMinZ;
    protected float f_PolyMaxZ;

    //--- Cached once in Init(). The old code re-read GetWorldSize() inside the placement loop, on
    //--- every one of up to 500 rolls per circle.
    protected float f_WorldSize;
    protected vector m_MapCenter;

    //--- Max distance from the map centre at which circle i may sit while still leaving a completable
    //--- chain behind it. Closed form, computed once:
    //---     Allow[n-1] = W/2 - r_{n-1}
    //---     Allow[i]   = min( W/2 - r_i , Allow[i+1] + reach(i+1) )
    //--- This is the INSCRIBED-CIRCLE approximation of the world box, so it is conservative: the real
    //--- box is a square and is more permissive near its diagonals. That is exactly what is wanted
    //--- here, because a_AllowRadius is only ever used to pick a seed (where being conservative means
    //--- being safe) and to gauge pressure. The ACCEPTANCE test is the exact CanChainComplete below.
    protected ref array<float> a_AllowRadius;

    static ref map<int, ref BattleRoyaleZone> m_Zones;

    //--- Counts every native surface query the generator makes, so the self test can report a
    //--- MEASURED cost instead of an estimated one.
    static int s_SurfaceCalls;

    //--- THE RADII ACTUALLY IN FORCE, which are a_StaticSizes unless allow_zone_size_flex grew one of
    //--- them for this match (see TryGrowLevel). EVERYTHING geometric reads this rather than
    //--- a_StaticSizes: GetStepReach, CanChainComplete, GetChainPressure, TryPlaceLevel,
    //--- SweepPlaceLevel and CommitChain. a_StaticSizes stays the admin's untouched input and is only
    //--- read to seed this array and to measure the growth against.
    //---
    //--- Filled in Init() BEFORE ComputeAllowRadii(), and that ordering is load-bearing rather than
    //--- tidy: ComputeAllowRadii, BuildFeasiblePOIList and PickSeedCenter are all DEFINED THROUGH
    //--- GetStepReach and CanChainComplete, so they read this array whether they mean to or not. With
    //--- it still NULL, GetStepReach returns 0 and a_AllowRadius collapses to W/2 - r_{n-1} - 4305 m
    //--- instead of 7579 m on ChernarusPlus - and BuildFeasiblePOIList then rejects nearly every POI.
    //--- No error and no warning; the numbers are simply wrong and plausible, the same failure texture
    //--- as the aliasing bug documented in ComputeAllowRadii.
    //---
    //--- Reused with Set() per seed attempt rather than reallocated: 8 seeds x 200 self-test runs is
    //--- 1600 allocations, and every reallocation is another window where the array is NULL.
    static ref array<float> s_ChainRadii;

    //--- Metres of growth this chain has already spent, against BR_ZONE_GROW_BUDGET_M.
    static float s_GrowthSpent;
    //--- How many levels grew, and by how much in total, for LogGeneratedChain and the self test.
    static int s_GrowthCount;

    void BattleRoyaleZone(BattleRoyaleZone parent = NULL)
    {
        m_ParentZone = parent;
    }

    void Init()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleZone Init()");

        m_Config = BattleRoyaleConfig.GetConfig();

        BattleRoyaleGameData m_GameData = m_Config.GetGameData();
        i_RoundDurationMinutes = m_GameData.round_duration_minutes;

        m_ZoneSettings = m_Config.GetZoneData();
        i_NumRounds = m_ZoneSettings.num_zones;
        f_ConstantShrink = m_ZoneSettings.constant_scale;
        i_ShrinkType = m_ZoneSettings.shrink_type;
        f_Eulers = m_ZoneSettings.shrink_base;
        f_Exponent = m_ZoneSettings.shrink_exponent;
        a_StaticSizes = m_ZoneSettings.static_sizes;
        a_StaticTimers = m_ZoneSettings.static_timers;
        a_MinPlayers = m_ZoneSettings.min_players;
        b_EndInVillages = m_ZoneSettings.end_in_villages;
        a_avoidType = m_ZoneSettings.end_avoid_type;
        a_avoidCity = m_ZoneSettings.end_avoid_city;
        f_MinLandFraction = m_ZoneSettings.zone_min_land_fraction;

        b_DeriveLadder = m_ZoneSettings.derive_zone_ladder;
        f_MetresPerPlayer = m_ZoneSettings.zone_metres_per_player;
        i_MinPlayersFloor = m_ZoneSettings.zone_min_players_floor;
        f_LootDensityWeight = m_ZoneSettings.zone_loot_density_weight;
        f_LootFactorMin = m_ZoneSettings.zone_loot_factor_min;
        f_LootFactorMax = m_ZoneSettings.zone_loot_factor_max;

        m_PlayArea = new BattleRoyalePlayArea(Vector(0,0,0), 0.0);

        //--- Cache the world geometry before anything that searches in it.
        f_WorldSize = GetGame().GetWorld().GetWorldSize();
        m_MapCenter = "0 0 0";
        m_MapCenter[0] = f_WorldSize / 2;
        m_MapCenter[2] = f_WorldSize / 2;

        //--- BEFORE ComputeAllowRadii - see the s_ChainRadii field comment for what a NULL array does
        //--- to it. Everything geometric reads this array, so it has to be a valid copy of the static
        //--- sizes from the first moment anything can ask.
        ResetChainRadii();
        ComputeAllowRadii();

		// Convert final_zone_polygon strings to vectors and check if position is inside the polygon
		if (m_ZoneSettings.restrict_final_zone)
		{
			polygon_vertices = new array<vector>();
			foreach(string v : m_ZoneSettings.final_zone_polygon)
			{
				polygon_vertices.Insert(v.ToVector());
			}

			//--- Bounding box, once, for the polygon sampling in PickSeedCenter.
			f_PolyMinX = float.MAX;
			f_PolyMaxX = float.LOWEST;
			f_PolyMinZ = float.MAX;
			f_PolyMaxZ = float.LOWEST;

			foreach(vector vtx : polygon_vertices)
			{
				f_PolyMinX = Math.Min(f_PolyMinX, vtx[0]);
				f_PolyMaxX = Math.Max(f_PolyMaxX, vtx[0]);
				f_PolyMinZ = Math.Min(f_PolyMinZ, vtx[2]);
				f_PolyMaxZ = Math.Max(f_PolyMaxZ, vtx[2]);
			}
		}

        LogConfiguredZoneWindow();

        //--- Generation cannot fail any more, but keep the placeholder area above rather than storing
        //--- NULL so a misconfiguration so severe that no chain exists at all still cannot null-deref.
        BattleRoyalePlayArea generated_area = GetBattleRoyalePlayAreas( i_NumRounds - GetZoneNumber() );
        if(generated_area)
            m_PlayArea = generated_area;
    }

    //--- The three static_* arrays are ordered SMALLEST ZONE FIRST: index 0 is the tiny final circle
    //--- and the last index is the widest opening circle. num_zones therefore selects that many
    //--- tiers from the small end, so lowering it shortens a match by dropping the LARGEST circles
    //--- while always keeping the tight endgame one. Trailing entries being unused is by design;
    //--- an array SHORTER than num_zones is a real misconfiguration and is caught per-lookup.
    static bool s_LoggedZoneWindow;

    protected void LogConfiguredZoneWindow()
    {
        //--- Init() runs once per zone object; the settings are process-wide, so say this once.
        if(s_LoggedZoneWindow)
            return;

        s_LoggedZoneWindow = true;

        BattleRoyaleUtils.Info("[BattleRoyaleZone] num_zones = " + i_NumRounds + " -> using zone_settings entries [0.." + (i_NumRounds - 1) + "] (smallest zone first).");

        LogUnusedTail("static_sizes", a_StaticSizes.Count());
        LogUnusedTail("static_timers", a_StaticTimers.Count());
        LogUnusedTail("min_players", a_MinPlayers.Count());
    }

    protected void LogUnusedTail(string setting_name, int entry_count)
    {
        if(entry_count > i_NumRounds)
            BattleRoyaleUtils.Info("[BattleRoyaleZone] zone_settings." + setting_name + " has " + entry_count + " entries; [" + i_NumRounds + ".." + (entry_count - 1) + "] are unused at num_zones = " + i_NumRounds + ". Raise num_zones to play them.");
        else if(entry_count < i_NumRounds)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone] zone_settings." + setting_name + " has only " + entry_count + " entries but num_zones is " + i_NumRounds + "! Zones beyond entry " + (entry_count - 1) + " have no configuration.");
    }

    static BattleRoyaleZone GetZone(int x = 1)
    {
        BattleRoyaleZone m_Zone;

        if(!m_Zones)
        {
            m_Zones = new map<int, ref BattleRoyaleZone>();
        }

        int z_Index = x - 1;

        if(!m_Zones.Contains(z_Index))
        {
            BattleRoyaleUtils.Trace("[BattleRoyaleZone] Create zone " + z_Index);
            if(z_Index > 0)
            {
                //m_Zones[z_Index] = new BattleRoyaleZone(m_Zones[z_Index - 1]);
                //
                //--- RECURSE for the parent rather than reading m_Zones directly. A zone's SETTINGS
                //--- INDEX is derived by walking its parent chain (GetZoneNumber), so a zone built with
                //--- a NULL parent silently believes it is zone 1 and reads the wrong radius, timer and
                //--- min_players. The map read this replaces returned exactly that whenever anybody
                //--- asked for zone 3 before zone 2 - no error, no warning, just a plausible wrong
                //--- answer. Every caller happens to ask in ascending order today, and #284 added two
                //--- more that also do, which is precisely the kind of invariant that holds until it
                //--- does not. Depth is num_zones, i.e. under ten.
                m_Zone = new BattleRoyaleZone(GetZone(z_Index));
            } else {
                // First zone
                //m_Zones[0] = new BattleRoyaleZone;
                m_Zone = new BattleRoyaleZone;
                z_Index = 0;
            }
            m_Zone.Init();
            m_Zones.Insert(z_Index, m_Zone);
            return m_Zone;
        } else {
            return m_Zones.Get(z_Index);
        }
    }

    BattleRoyalePlayArea GetArea()
    {
        return m_PlayArea;
    }

    BattleRoyaleZone GetParent()
    {
        return m_ParentZone;
    }

    void OnActivate(notnull array<PlayerBase> players)
    {
        //This method is run before GetArea() is ever called. This can be used to change the play area size based on players.
        //Note that this on the main thread, therefore it must be performant.
        //we can look at CreatePlayArea / CreatePlayRadius & Init methods for examples of zone size creation
    }

    //returns which # zone this is ( 1 for the first zone )
    int GetZoneNumber()
    {
        int number = 1;
        BattleRoyaleZone parent = m_ParentZone;
        while(parent)
        {
            parent = parent.GetParent();
            number++;
        }
        return number;
    }

    //--- Index into the smallest-zone-first settings arrays for this zone. Zone 1 (the widest,
    //--- first-played circle) maps to the highest index; the last zone maps to index 0.
    protected int GetZoneSettingsIndex()
    {
        return i_NumRounds - GetZoneNumber();
    }

    //--- Is the derived timing switched on AND applicable? Gated on shrink_type 3 as well as the
    //--- setting, because static_timers - the thing being replaced - only applies to that mode, so
    //--- letting the flag through for the others would silently override round_duration_minutes.
    protected bool IsDerivedTimingActive()
    {
        if(!m_ZoneSettings || !m_ZoneSettings.derive_timers_from_geometry)
            return false;

        return (i_ShrinkType == 3);
    }

    /**
     *  How long the round that plays this circle runs, in seconds.
     *
     *  `is_opening_round` says this circle is the FIRST one played this match - i.e. its zone number is
     *  the dynamic starting zone. That is not a property of the circle, it is a property of the match,
     *  which is why it is a parameter rather than something this method works out for itself.
     *
     *  ⚠️ IT MUST STAY A PARAMETER. BattleRoyaleState.GetDynamicStartingZone's duration bound calls
     *  this once per candidate tier to price a whole match; if this method asked
     *  GetDynamicStartingZone which tier was the opening one, the two would call each other forever.
     *  The three callers all already know: 6_BattleRoyaleRound compares its own zone_num against the
     *  starting zone two lines below, 5_BattleRoyaleStartMatch is by definition pricing that round, and
     *  the duration bound is iterating candidates.
     */
    int GetZoneTimer(bool is_opening_round = false)
    {
        if (i_ShrinkType ==  3)
        {
            //--- Range-check the index we actually use, not the 1-based zone number: any
            //--- static_timers shorter than num_zones used to pass the old guard and read out of range.
            int timer_index = GetZoneSettingsIndex();
            if(timer_index < 0 || timer_index >= a_StaticTimers.Count())
            {
                //--- Warn, not Error: BattleRoyaleUtils.Error raises a VM exception, so the `return
                //--- 300` below never ran and a short static_timers halted the script VM instead of
                //--- degrading - the exact opposite of what the fallback was written to do.
                BattleRoyaleUtils.Warn("Not enough static timers! (zone " + GetZoneNumber() + " wants index " + timer_index + ", have " + a_StaticTimers.Count() + ")");
                return 300;
            }

            //--- THE OPENING ROUND IS A LOOT ROUND (#284 point 4), and this branch has to come FIRST.
            //---
            //--- It is the one round with no inbound travel to price: 4_BattleRoyalePrepare spawns
            //--- everybody inside the very circle this round is going to lock, so the "travel from
            //--- circle i+1 into circle i" model below has nothing to measure. What it costs is looting
            //--- plus the sprint a spawned player still owes inside that circle - see
            //--- BR_ZONE_TIMER_LOOT_SECONDS and CommitChain, which precomputes it for every index.
            //---
            //--- ⚠️ THIS ALSO FIXES A REAL BUG, and it is why "just exclude index n-1" below is not
            //--- enough. Index n-1 is the opening round ONLY when the match starts at zone 1. With a
            //--- dynamic starting zone the opening round is index n-Z, and it was being handed
            //--- derived[n-Z] - the travel from circle n-Z+1, a circle that was SKIPPED and never
            //--- played. A plausible number for a journey nobody made.
            if(IsDerivedTimingActive() && is_opening_round)
            {
                float opening = GetOpeningTimer(timer_index);
                if(opening > 0)
                    return Math.Round(opening);
            }

            //--- Derived timing (#241 part 3), when it is on and this round has a geometry to derive
            //--- from. Note it deliberately does NOT add GetDurationOffset: the derivation already uses
            //--- the real placed distance and the radii in force, so the offset's travel bonus and its
            //--- growth term would both be counted twice.
            //---
            //--- Index n-1 is still excluded, and now only as a BACKSTOP. It has no predecessor circle,
            //--- so CommitChain leaves its derived entry at 0; taken literally that would clamp to
            //--- BR_ZONE_TIMER_MIN_SECONDS and collapse the one round where players are spread over the
            //--- whole map. In practice the loot branch above already claimed it whenever the match
            //--- starts at zone 1, which is the only way that index is ever played - so what reaches
            //--- here is a caller that did not know it was pricing the opening round, and
            //--- static_timers is the right answer for it.
            if(IsDerivedTimingActive() && timer_index < (i_NumRounds - 1))
            {
                float derived = GetDerivedTimer(timer_index);
                if(derived > 0)
                    return Math.Round(derived);   //--- a travel budget: never truncate it downward
            }

            return a_StaticTimers[timer_index] + GetDurationOffset(timer_index);
        }

        return 60 * i_RoundDurationMinutes;
    }

    int GetZoneMinPlayers()
    {
        int min_players_index = GetZoneSettingsIndex();

        //--- The derived table when it is on and this circle has an entry (#284 point 1). Falls
        //--- through to the admin's a_MinPlayers otherwise, so a boot where generation never ran -
        //--- and therefore where BuildDerivedLadder never ran either - still answers something sane
        //--- rather than zero.
        if(b_DeriveLadder && s_PlayAreaMinPlayers && min_players_index >= 0 && min_players_index < s_PlayAreaMinPlayers.Count())
            return s_PlayAreaMinPlayers.Get(min_players_index);

        if(min_players_index < 0 || min_players_index >= a_MinPlayers.Count())
        {
            //--- 0 makes GetDynamicStartingZone settle on zone 1, so a short min_players degrades
            //--- to a full-length match instead of an arbitrary one. Warn, not Error, for the same
            //--- reason as GetZoneTimer above: Error would unwind before the fallback returns.
            BattleRoyaleUtils.Warn("Not enough min players! (zone " + GetZoneNumber() + " wants index " + min_players_index + ", have " + a_MinPlayers.Count() + ")");
            return 0;
        }

        return a_MinPlayers[min_players_index];
    }

    //--- Extra seconds granted to a round whose circle sits far from the one before it, so players
    //--- can actually cross the gap. Indexed exactly like the settings arrays.
    protected float GetDurationOffset(int play_area_index)
    {
        if(!s_PlayAreaDurationOffsets)
            return 0;

        if(play_area_index < 0 || play_area_index >= s_PlayAreaDurationOffsets.Count())
            return 0;

        return s_PlayAreaDurationOffsets[play_area_index];
    }

    //--- The geometry-derived length of the round that plays this circle. Same indexing as everything
    //--- else here. 0 means "nothing derived for this index", which is the honest answer for the opening
    //--- round and for a boot where CommitChain never ran; callers fall back to static_timers.
    float GetDerivedTimer(int play_area_index)
    {
        if(!s_PlayAreaDerivedTimers)
            return 0;

        if(play_area_index < 0 || play_area_index >= s_PlayAreaDerivedTimers.Count())
            return 0;

        return s_PlayAreaDerivedTimers[play_area_index];
    }

    //--- What the round playing this circle would cost if it were the OPENING round. Same indexing as
    //--- everything else here. 0 means "nothing computed for this index", i.e. CommitChain never ran.
    float GetOpeningTimer(int play_area_index)
    {
        if(!s_PlayAreaOpeningTimers)
            return 0;

        if(play_area_index < 0 || play_area_index >= s_PlayAreaOpeningTimers.Count())
            return 0;

        return s_PlayAreaOpeningTimers[play_area_index];
    }

    //--- POIs inside this circle. -1 means "not counted", which is the honest answer on a boot where
    //--- BuildDerivedLadder never ran - distinct from a genuine 0, i.e. a circle over empty ground.
    static int GetPOICount(int play_area_index)
    {
        if(!s_PlayAreaPOICount)
            return -1;

        if(play_area_index < 0 || play_area_index >= s_PlayAreaPOICount.Count())
            return -1;

        return s_PlayAreaPOICount.Get(play_area_index);
    }

    //--- The derived min_players for this circle, or -1 when the ladder was never derived. Distinct
    //--- from GetZoneMinPlayers, which answers with the admin's table when the feature is off; this one
    //--- is for diagnostics that want to show BOTH.
    static int GetDerivedMinPlayers(int play_area_index)
    {
        if(!s_PlayAreaMinPlayers)
            return -1;

        if(play_area_index < 0 || play_area_index >= s_PlayAreaMinPlayers.Count())
            return -1;

        return s_PlayAreaMinPlayers.Get(play_area_index);
    }

    //--- How much circle `play_area_index` grew for this match, in metres. 0 when flex is off or when
    //--- this circle was placed at its static size. Diagnostics only.
    //---
    //--- Measured against the COMMITTED play area rather than against s_ChainRadii, and that is the point
    //--- rather than a detail: m_PlayAreas is immutable once CommitChain has run, while s_ChainRadii is
    //--- working state that RunSelfTest churns 200 times over afterwards. Reading the committed value
    //--- means this answer cannot go stale no matter what else touches the generator.
    float GetRadiusGrowth(int play_area_index)
    {
        float static_radius = 0;
        float committed;
        BattleRoyalePlayArea area;

        if(!m_PlayAreas || play_area_index < 0 || play_area_index >= m_PlayAreas.Count())
            return 0;

        if(a_StaticSizes && play_area_index < a_StaticSizes.Count())
            static_radius = a_StaticSizes[play_area_index];

        if(static_radius <= 0)
            return 0;

        //--- Element into a local before the call, per ComputeAllowRadii's rule.
        area = m_PlayAreas.Get(play_area_index);
        if(!area)
            return 0;

        committed = area.GetRadius();
        if(committed <= static_radius)
            return 0;

        return committed - static_radius;
    }

    bool IsInZone(float x, float z)
    {
        vector center = GetArea().GetCenter();

        float d = (Math.Pow(x - center[0], 2) + Math.Pow(z - center[2], 2));
        float radius_pow = Math.Pow(GetArea().GetRadius(), 2);

        BattleRoyaleUtils.Trace(d);
        BattleRoyaleUtils.Trace(radius_pow);

        return (d < radius_pow);
    }

    //=====================================================================================
    //--- Geometry core.
    //---
    //--- Everything in this block is PURE ARITHMETIC - not one native call - and that is the point.
    //--- The old generator discovered that a position was hopeless by failing to extend it 500 times;
    //--- these functions answer the same question outright, before any surface is ever queried.
    //---
    //--- The fact it rests on: the world-fit boxes B_i = [r_i, W - r_i]^2 are NESTED and all share
    //--- the map centre. For nested convex sets containing a common point, moving along the segment
    //--- toward that point weakly decreases the distance to EVERY one of them. So "step the maximum
    //--- allowed straight toward the map centre" is provably the optimal continuation of a chain, not
    //--- a heuristic - which makes CanChainComplete an exact oracle rather than an approximation.
    //---
    //--- Two consequences shape the whole subsystem:
    //---   1. Used as an acceptance test, every circle it accepts is provably extendable.
    //---   2. The first greedy step from any accepted position is itself always acceptable - the
    //---      "witness step". So every level has a move that cannot be rejected on geometric grounds,
    //---      and placement cannot dead-end. Termination is proved, not budgeted.
    //---
    //--- NOTE the nesting needs only that the radii be STRICTLY INCREASING - not that they be the ones
    //--- in static_sizes. That is what lets TryGrowLevel vary one per match without weakening any of
    //--- the above, and it is why every function here reads s_ChainRadii. What it does add is a
    //--- maintenance constraint: the radii in force when a position is accepted must still be in force
    //--- when its child takes its witness step, because that step is accepted with no FitsWorld check.
    //=====================================================================================

    //--- The radius in force for circle `i`. One accessor so the a_StaticSizes fallback lives in one
    //--- place: s_ChainRadii is filled in Init() before anything can ask, but a caller reached during a
    //--- misconfigured boot must degrade to the static value rather than to zero.
    protected float GetChainRadius(int i)
    {
        if(s_ChainRadii && i >= 0 && i < s_ChainRadii.Count())
            return s_ChainRadii.Get(i);

        if(a_StaticSizes && i >= 0 && i < a_StaticSizes.Count())
            return a_StaticSizes[i];

        return 0;
    }

    //--- Back to the admin's sizes, for every level. Called from Init() and from the top of each seed
    //--- attempt, so a seed re-roll never inherits the previous attempt's growth.
    protected void ResetChainRadii()
    {
        //--- One declaration per name per METHOD scope in EnfusionScript, so every local is up here
        //--- rather than at first use - the same shape as Validate() in BattleRoyaleZoneData.
        int i;
        float static_radius;

        if(!s_ChainRadii)
            s_ChainRadii = new array<float>();

        //--- Grown to length once, then only ever Set() - see the field comment on why this is not
        //--- reallocated per attempt.
        while(s_ChainRadii.Count() < i_NumRounds)
        {
            s_ChainRadii.Insert(0);
        }

        for(i = 0; i < s_ChainRadii.Count(); i++)
        {
            //--- Array read on its own line before the Set() call, per ComputeAllowRadii's rule.
            static_radius = 0;
            if(a_StaticSizes && i < a_StaticSizes.Count())
                static_radius = a_StaticSizes[i];

            s_ChainRadii.Set(i, static_radius);
        }

        s_GrowthSpent = 0;
        s_GrowthCount = 0;
    }

    //--- Back to the admin's sizes for `level` and everything above it. Called when BuildChain
    //--- backtracks: levels above the one being re-rolled are no longer placed, so any growth they were
    //--- granted has to go with them or it accumulates across retries.
    protected void ResetChainRadiiFrom(int level)
    {
        int i;
        float static_radius;
        float grown;

        if(!s_ChainRadii)
            return;

        for(i = level; i < s_ChainRadii.Count(); i++)
        {
            static_radius = 0;
            if(a_StaticSizes && i < a_StaticSizes.Count())
                static_radius = a_StaticSizes[i];

            //--- Refund the budget, or a chain that backtracked a lot would run out of growth it never
            //--- actually kept. Both reads land in locals before the Set().
            grown = s_ChainRadii.Get(i);
            if(grown > static_radius)
            {
                s_GrowthSpent = s_GrowthSpent - (grown - static_radius);
                s_GrowthCount = s_GrowthCount - 1;
            }

            s_ChainRadii.Set(i, static_radius);
        }

        if(s_GrowthSpent < 0)
            s_GrowthSpent = 0;
        if(s_GrowthCount < 0)
            s_GrowthCount = 0;
    }

    //--- How far circle `level` may move from circle `level-1`. Containment alone would permit the
    //--- full (r_i - r_{i-1}); BR_ZONE_REACH_PERCENT is how much of that the chain plans on.
    protected float GetStepReach(int level)
    {
        if(level < 1 || level >= i_NumRounds)
            return 0;

        //--- Each read on its own line, then the arithmetic - the reads must not share an expression
        //--- with anything that indexes another array. See ComputeAllowRadii.
        float radius = GetChainRadius(level);
        float parent_radius = GetChainRadius(level - 1);

        return BR_ZONE_REACH_PERCENT * (radius - parent_radius);
    }

    //--- Whole circle inside the world square. Four separate `if`s rather than one compound
    //--- condition, because EnfusionScript will not accept a multi-line `if`.
    protected bool FitsWorld(vector p, float radius)
    {
        if(p[0] < radius)
            return false;
        if(p[2] < radius)
            return false;
        if((p[0] + radius) > f_WorldSize)
            return false;
        if((p[2] + radius) > f_WorldSize)
            return false;

        return true;
    }

    //--- Move from_pos toward to_pos by at most max_step, stopping exactly on to_pos if it is nearer
    //--- than that. Works purely in x/z and always returns y = 0: callers that need a real height set
    //--- it from SurfaceY once the position is final.
    protected vector StepToward(vector from_pos, vector to_pos, float max_step)
    {
        vector result = "0 0 0";
        float dx = to_pos[0] - from_pos[0];
        float dz = to_pos[2] - from_pos[2];
        float d = Math.Sqrt((dx * dx) + (dz * dz));
        float scale;

        if(d <= max_step || d <= 0.001)
        {
            result[0] = to_pos[0];
            result[2] = to_pos[2];
            return result;
        }

        scale = max_step / d;
        result[0] = from_pos[0] + (dx * scale);
        result[2] = from_pos[2] + (dz * scale);
        return result;
    }

    //--- THE ORACLE. Given circle `level` placed at `pos`, can every remaining circle still be
    //--- placed legally? Walks the provably-optimal continuation and checks each world-fit box.
    //--- At most i_NumRounds iterations, zero native calls.
    protected bool CanChainComplete(vector pos, int level)
    {
        vector p = pos;
        int i;
        float reach;
        float radius;

        if(!a_StaticSizes)
            return false;

        for(i = level + 1; i < i_NumRounds; i++)
        {
            if(i >= a_StaticSizes.Count())
                return false;

            //--- Both of these into locals BEFORE the calls that consume them. GetStepReach and
            //--- GetChainRadius each index s_ChainRadii, so folding either into the StepToward or
            //--- FitsWorld argument list is exactly the shape ComputeAllowRadii documents as reading the
            //--- wrong array. This is the oracle; a silent misread here is an illegal chain.
            reach = GetStepReach(i);
            p = StepToward(p, m_MapCenter, reach);

            radius = GetChainRadius(i);
            if(!FitsWorld(p, radius))
                return false;
        }

        return true;
    }

    //--- Closed-form Allow[] - see the field comment. Cheap enough to just recompute per zone object.
    protected void ComputeAllowRadii()
    {
        int i;
        float box_allow;
        float chain_allow;
        float prev_allow;
        float step_reach;

        a_AllowRadius = new array<float>();

        if(i_NumRounds < 1 || !a_StaticSizes || a_StaticSizes.Count() < i_NumRounds)
            return;

        for(i = 0; i < i_NumRounds; i++)
        {
            a_AllowRadius.Insert(0);
        }

        a_AllowRadius.Set(i_NumRounds - 1, (f_WorldSize / 2) - a_StaticSizes[i_NumRounds - 1]);

        //--- The array read is MATERIALISED INTO A LOCAL before the method call, and that is not
        //--- stylistic - it is the whole bug. Written as one expression:
        //---     chain_allow = a_AllowRadius.Get(i + 1) + GetStepReach(i + 1);
        //--- the read silently yields entries of a_StaticSizes instead. Measured 2026-08-11 on
        //--- ChernarusPlus: allow[0] came out 119.875, which is exactly a_StaticSizes[1] + reach(1),
        //--- where the correct answer is 7579 - and every other entry matched a_StaticSizes[i+1] +
        //--- reach(i+1) too. Splitting the same arithmetic across two locals fixes it, with the []
        //--- operator or with Get() alike, so it is the array read sharing an expression with a call
        //--- that breaks, not the subscript syntax. No error, no warning: the numbers are simply
        //--- wrong and plausible. Keep every array read on its own line here.
        for(i = i_NumRounds - 2; i >= 0; i--)
        {
            prev_allow = a_AllowRadius.Get(i + 1);
            step_reach = GetStepReach(i + 1);

            box_allow = (f_WorldSize / 2) - a_StaticSizes[i];
            chain_allow = prev_allow + step_reach;

            a_AllowRadius.Set(i, Math.Min(box_allow, chain_allow));
        }
    }

    //--- How much of this step's reach the chain actually OWES, as a fraction in [0, 1]. 0 means the
    //--- circle could stay exactly where its parent is and the chain would still complete; 1 means
    //--- only a full-reach step straight at the map centre will do.
    //---
    //--- The bisection needs no failure branch because hi = reach is ALWAYS a valid answer: the
    //--- parent was itself accepted by CanChainComplete, and the first step of the continuation that
    //--- proved is exactly this full-reach step toward the centre.
    protected float GetChainPressure(vector parent_center, int level)
    {
        float reach = GetStepReach(level);
        float lo = 0;
        float hi;
        float mid;
        int i;
        vector probe;
        //--- Materialised once, outside both FitsWorld conditions. CanChainComplete on the far side of
        //--- the && indexes s_ChainRadii, so `FitsWorld(probe, s_ChainRadii[level]) && CanChainComplete(...)`
        //--- would put an array read in an expression with a call that indexes the same array - the
        //--- ComputeAllowRadii shape. Not a place to rely on "probably fine".
        float level_radius = GetChainRadius(level);

        if(reach <= 0)
            return 0;

        if(!a_StaticSizes || level >= a_StaticSizes.Count())
            return 0;

        //--- No movement needed at all is the common case; answer it without bisecting.
        probe = StepToward(parent_center, m_MapCenter, 0);
        if(FitsWorld(probe, level_radius) && CanChainComplete(probe, level))
            return 0;

        hi = reach;
        for(i = 0; i < 5; i++)
        {
            mid = (lo + hi) * 0.5;
            probe = StepToward(parent_center, m_MapCenter, mid);

            if(FitsWorld(probe, level_radius) && CanChainComplete(probe, level))
                hi = mid;
            else
                lo = mid;
        }

        return hi / reach;
    }

    //=====================================================================================
    //--- Surface predicates. These are the only things here that cost native calls.
    //=====================================================================================

    //--- Counting wrapper. Every sea query in the generator goes through this one.
    protected bool IsSea(float x, float z)
    {
        s_SurfaceCalls++;
        return GetGame().SurfaceIsSea(x, z);
    }

    //--- Surface types a circle centre may never sit on, beyond sea and pond. Static and public so
    //--- 6_BattleRoyaleRound.IsSafeForAirdrop can share it instead of hardcoding its own copy.
    static bool IsBadSurfaceType(float x, float z)
    {
        BattleRoyaleConfig config = BattleRoyaleConfig.GetConfig();
        if(!config)
            return false;

        BattleRoyaleZoneData settings = config.GetZoneData();
        if(!settings || !settings.avoid_surface_types || settings.avoid_surface_types.Count() == 0)
            return false;

        string surface_type;
        GetGame().SurfaceGetType(x, z, surface_type);

        return (settings.avoid_surface_types.Find(surface_type) != -1);
    }

    //--- Fraction of this circle that is dry land, in [0, 1].
    //---
    //--- Below BR_ZONE_SMALL_CIRCLE_R this degrades to the old strict single-point test, because a
    //--- 35 m circle centred 20 m offshore genuinely is unplayable. Above it, sampling the disc is
    //--- what makes coastal and island maps workable at all: a 3375 m circle that is 90% dry land
    //--- used to be rejected outright for having its exact centre pixel in the water.
    //---
    //--- SurfaceIsPond is deliberately NOT tested for large circles - a pond cannot spoil a circle
    //--- kilometres across, and testing it would double the native count for nothing.
    protected float GetLandScore(vector center, float radius)
    {
        float total = 0;
        float land = 0;
        float ring_radius;
        float phase;
        float angle;
        float sx;
        float sz;
        float rings_f = BR_ZONE_LAND_RINGS;
        int ring;
        int step;

        if(radius <= BR_ZONE_SMALL_CIRCLE_R)
        {
            if(IsSea(center[0], center[2]))
                return 0;
            if(GetGame().SurfaceIsPond(center[0], center[2]))
                return 0;
            if(IsBadSurfaceType(center[0], center[2]))
                return 0;

            return 1.0;
        }

        total = total + 1;
        if(!IsSea(center[0], center[2]))
            land = land + 1;

        for(ring = 1; ring <= BR_ZONE_LAND_RINGS; ring++)
        {
            //--- Equal-AREA annulus midpoints. Spacing the rings evenly by radius instead would
            //--- under-sample the outer band, which is most of a large circle's area.
            ring_radius = radius * Math.Sqrt((ring - 0.5) / rings_f);
            phase = Math.RandomFloat(0, Math.PI2);

            for(step = 0; step < BR_ZONE_LAND_PER_RING; step++)
            {
                angle = phase + ((step * Math.PI2) / BR_ZONE_LAND_PER_RING);
                sx = center[0] + (ring_radius * Math.Sin(angle));
                sz = center[2] + (ring_radius * Math.Cos(angle));

                total = total + 1;
                if(!IsSea(sx, sz))
                    land = land + 1;
            }
        }

        if(total <= 0)
            return 0;

        return land / total;
    }

    //--- Strict test for the final circle's centre: it is where the match is decided, so it gets no
    //--- tolerance for water at all, and it must also honour the configured polygon.
    protected bool IsFinalZoneCenterSafe(vector p)
    {
        if(IsSea(p[0], p[2]))
            return false;
        if(GetGame().SurfaceIsPond(p[0], p[2]))
            return false;
        if(IsBadSurfaceType(p[0], p[2]))
            return false;

        return IsValidFinalZonePosition(p);
    }

    //=====================================================================================
    //--- POI feasibility
    //=====================================================================================

    //--- POIs that can actually seed a chain, filtered by the oracle. This REMOVES NO USABLE POI:
    //--- every one it rejects was already a coin flip that ended in the server shutting itself down.
    static ref array<ref array<float>> s_FeasiblePOI;
    static int s_POIRawCount;

    void BuildFeasiblePOIList()
    {
        int i;
        int rejected;
        vector probe = "0 0 0";

        if(s_FeasiblePOI)
            return;

        InitializePOIs();

        s_FeasiblePOI = new array<ref array<float>>();

        if(!s_POI || !a_StaticSizes || a_StaticSizes.Count() == 0)
            return;

        for(i = 0; i < s_POI.Count(); i++)
        {
            array<float> poi = s_POI.Get(i);
            if(!poi || poi.Count() < 2)
                continue;

            //--- s_POI holds 2-element [x, z] pairs, read back as poi[0] / poi[1].
            probe[0] = poi[0];
            probe[2] = poi[1];

            if(!FitsWorld(probe, a_StaticSizes[0]))
                continue;
            if(!CanChainComplete(probe, 0))
                continue;

            s_FeasiblePOI.Insert(poi);
        }

        rejected = s_POI.Count() - s_FeasiblePOI.Count();

        BattleRoyaleUtils.Info("[BattleRoyaleZone] POIs: " + s_POIRawCount + " in CfgWorlds, " + s_POI.Count() + " after the avoid lists, " + s_FeasiblePOI.Count() + " chain-feasible (usable disc r=" + GetSeedDiscRadius() + " m around " + m_MapCenter[0] + " " + m_MapCenter[2] + ").");

        if(rejected > 0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone] " + rejected + " POIs are too far from the map centre for a " + a_StaticSizes[i_NumRounds - 1] + " m opening circle to still fit this " + f_WorldSize + " m world. Lower static_sizes for this map (or set scale_sizes_to_world), or raise num_zones to buy more reach.");

        if(s_FeasiblePOI.Count() == 0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone] NO POI on this map can seed a chain - the final circle will be placed by disc sampling around the map centre instead. end_in_villages cannot be honoured here.");
    }

    //--- Radius of the disc around the map centre inside which a final circle is guaranteed to be
    //--- extendable. Negative would mean the largest circle cannot fit the world at all, which
    //--- BattleRoyaleZoneData.Validate() clamps away before generation ever runs.
    protected float GetSeedDiscRadius()
    {
        if(!a_AllowRadius || a_AllowRadius.Count() == 0)
            return 0;

        return a_AllowRadius.Get(0);
    }

    static ref array<ref BattleRoyalePlayArea> m_PlayAreas;

    //--- Parallel to m_PlayAreas: extra round seconds earned by the travel into each circle, plus any
    //--- extra ground a grown circle added (#241). Static for the same reason m_PlayAreas is - the
    //--- circles are generated once per process and every zone object reads the same set.
    //---
    //--- Written ONLY by CommitChain, from the finished chain. Never accumulated during placement: see
    //--- CommitChain's header for what RunSelfTest does to an array that is.
    static ref array<float> s_PlayAreaDurationOffsets;

    //--- Also parallel to m_PlayAreas: what each round's length WOULD be if derived from the geometry
    //--- rather than read from static_timers (#241 part 3). Filled unconditionally so the diag zone
    //--- table can show it either way; only consumed when derive_timers_from_geometry is on.
    //---
    //--- Index n-1 - the opening round - is always 0 and unused. It has no predecessor circle, so there
    //--- is no travel to derive from; GetZoneTimer keeps that round on static_timers.
    static ref array<float> s_PlayAreaDerivedTimers;

    //--- What each round would cost IF IT WERE THE OPENING ROUND (#284 point 4): loot allowance plus
    //--- the sprint a spawned player still owes inside the circle, and no inbound travel at all -
    //--- 4_BattleRoyalePrepare spawns everybody inside the circle the opening round is going to lock.
    //---
    //--- Filled for EVERY index, unconditionally, in CommitChain. Which index is the opening one is not
    //--- known until the countdown (it is the dynamic starting zone), and CommitChain is the only place
    //--- allowed to compute a timing term - see its header - so the answer is precomputed for all of
    //--- them and GetZoneTimer picks. It also means the diag zone table can show the column whichever
    //--- tier the match ends up opening on.
    static ref array<float> s_PlayAreaOpeningTimers;

    //--- POIs inside each placed circle, and the min_players derived from that plus the radius
    //--- (#284 point 1). Parallel to m_PlayAreas and static for the same reason it is: the circles are
    //--- generated once per process and every zone object reads the same set.
    //---
    //--- Written ONLY by BuildDerivedLadder, from the committed play areas, once per process. Not from
    //--- CommitChain: these are not timing terms, and s_POI is only guaranteed populated after
    //--- BuildFeasiblePOIList has run.
    static ref array<int> s_PlayAreaPOICount;
    static ref array<int> s_PlayAreaMinPlayers;

    //--- Lootable BUILDINGS inside each placed circle - the loot-density input that actually works.
    //--- Filled by CensusBuildings; -1 everywhere when the census was skipped or came back empty, in
    //--- which case BuildDerivedLadder falls back to the POI count and says so.
    static ref array<int> s_PlayAreaBuildings;

    //--- Census diagnostics, reported once at boot: cells probed, buildings found, milliseconds spent.
    //--- The cost is worth stating out loud because it is the one thing this feature adds to boot.
    static int s_CensusCells;
    static int s_CensusFound;
    static int s_CensusMs;

    //=====================================================================================
    //--- Placement.
    //---
    //--- BuildChain seeds circle 0 and walks levels 1..n-1 forward, backtracking to re-roll a parent
    //--- whenever a level cannot be placed. TryPlaceLevel escalates through three progressively
    //--- looser tiers, then one deterministic sweep, then the witness step - which the geometry core
    //--- guarantees is always acceptable, and which is why none of this can dead-end.
    //---
    //--- What this replaced: a single forward pass that drew 500 random candidates per circle and,
    //--- on running out, called GetGame().RequestExit(0) to take the whole server down so it would
    //--- restart and roll again. That was not bad luck. The per-step travel budget telescopes to
    //--- REACH * (r_max - r_min), so a final circle seeded further than that from the region where
    //--- the opening circle can legally sit could NEVER be extended - the 500 attempts were provably
    //--- wasted, every time, and no amount of retrying would have helped.
    //=====================================================================================

    //--- Per-generation statistics. Logged at boot and reported by the self test.
    static ref array<int> s_LevelRetries;
    //--- [0]=T1 [1]=T2 [2]=T3 [3]=growth [4]=sweep [5]=witness step. Sized from BR_ZONE_TIER_SLOTS
    //--- rather than a literal, because RunSelfTest walks it with a second loop of its own.
    static ref array<int> s_TierUsage;
    static int s_TotalWork;
    static int s_Backtracks;
    static int s_DeepestBacktrack;
    static int s_SeedsUsed;

    protected void ResetGenerationStats()
    {
        int i;

        s_TotalWork = 0;
        s_Backtracks = 0;
        s_DeepestBacktrack = 0;
        s_SeedsUsed = 0;

        s_TierUsage = new array<int>();
        for(i = 0; i < BR_ZONE_TIER_SLOTS; i++)
        {
            s_TierUsage.Insert(0);
        }
    }

    //--- The straight-at-the-map-centre step the feasibility oracle is built on. Because the parent
    //--- was itself accepted by CanChainComplete, this exact step is the first move of the
    //--- continuation that proved it - so it is guaranteed to fit the world AND to leave the rest of
    //--- the chain completable. Only its LAND quality is unchecked, which is why it is the last
    //--- resort rather than the first choice.
    protected vector WitnessStep(int level, vector parent_center)
    {
        return StepToward(parent_center, m_MapCenter, GetStepReach(level));
    }

    //--- A systematic scan of the annulus, run once after the random tiers have all failed.
    //---
    //--- Deliberately NOT the primary mechanism. Accept-first pays ~2 rolls in the common case where
    //--- this pays all 96 every time; determinism would flatten the match-to-match variety; and a
    //--- deterministic best-pick as the main path would break backtracking outright, since re-rolling
    //--- a parent only makes progress if it can return a different answer. Its job is to establish
    //--- that the parent really is a dead end, so backtracking is triggered by evidence rather than
    //--- by another few hundred random rolls arriving at the same conclusion slowly.
    protected bool SweepPlaceLevel(int level, vector parent_center, float centre_dir, out vector placed)
    {
        //--- The radii IN FORCE, not the admin's - this level may already have grown. Each read on its
        //--- own line before the subtraction, per ComputeAllowRadii's rule.
        float radius = GetChainRadius(level);
        float parent_radius = GetChainRadius(level - 1);
        float span = radius - parent_radius;
        float steps_f = BR_ZONE_SWEEP_DISTANCES;
        float best_score = -1;
        float score;
        float angle;
        float dist;
        int a;
        int d;
        vector candidate = "0 0 0";
        vector best = "0 0 0";

        placed = "0 0 0";

        for(a = 0; a < BR_ZONE_SWEEP_ANGLES; a++)
        {
            angle = centre_dir + ((a * Math.PI2) / BR_ZONE_SWEEP_ANGLES);

            for(d = 1; d <= BR_ZONE_SWEEP_DISTANCES; d++)
            {
                dist = span * BR_ZONE_REACH_PERCENT * (d / steps_f);

                candidate[0] = parent_center[0] + (dist * Math.Sin(angle));
                candidate[2] = parent_center[2] + (dist * Math.Cos(angle));

                if(!FitsWorld(candidate, radius))
                    continue;
                if(!CanChainComplete(candidate, level))
                    continue;

                score = GetLandScore(candidate, radius);
                if(score <= best_score)
                    continue;

                best_score = score;
                best = candidate;
            }
        }

        //--- Zero land is no better than the witness step, which at least makes maximal progress.
        if(best_score <= 0)
            return false;

        best[1] = GetGame().SurfaceY(best[0], best[2]);
        placed = best;
        return true;
    }

    //=====================================================================================
    //--- PER-MATCH RADIUS FLEX (#241). #19 asked that when the preferred distance window cannot be
    //--- respected, the generator "determine the best between the zone maximum time and the zone size".
    //--- This is the size half; CommitChain pays for it with the time half.
    //---
    //--- A bigger radius is a bigger span, which is more reach and more freedom to sit off-centre - the
    //--- exact thing a squeezed level cannot get - and a bigger circle averages out water, so the land
    //--- gate is easier to clear. static_sizes is never touched: the growth lives in s_ChainRadii and
    //--- therefore only in this match's generated chain.
    //---
    //--- WHERE IT SITS IN THE LADDER IS THE ONE DESIGN DECISION HERE, AND IT WAS SETTLED BY MEASUREMENT
    //--- AFTER TWO WRONG ANSWERS. It is called from tier 0's failure path, before tier 2 loosens the bar.
    //---
    //--- The reasoning that put it later was that the three tiers loosen the ACCEPTANCE BAR over a fixed
    //--- annulus, while growth changes the ANNULUS ITSELF and permanently alters the match - so it should
    //--- be a last resort rather than a second choice. That is a fair description and it produced a
    //--- feature that never fired once:
    //---   * After tier 3 (build 1, gated on pressure > 0): growth 0 in 1000 placements on Sakhal, and
    //---     the path was reached 47 times. Every refusal was the pressure gate, because the failures
    //---     that get that far on Sakhal are WATER failures - the chain is not squeezed, so pressure is
    //---     0 by construction exactly when this is reached.
    //---   * After tier 3 (build 2, gate removed): still growth 0, path reached 62 times. The gate was
    //---     never the blocker. By tier 3 the ladder has already accepted 0.10 land over a 180 deg arc
    //---     and failed, so re-asking for f_MinLandFraction (0.60) is a STRICTER bar than the tier that
    //---     just failed. Growth could not have succeeded there whatever the gate said.
    //---   * After the SWEEP would be worse still, for the same reason squared.
    //--- So the only position where growth is a fair test is against tier 1's own window and tier 1's own
    //--- land bar, differing in the radius alone - which is also exactly #19's condition, "if the 25%/75%
    //--- can't be respected". If growth fails there, tiers 2 and 3 loosen the bar as they always did.
    //---
    //--- The cost objection to this position is real and was measured rather than assumed: growth adds up
    //--- to BR_ZONE_GROW_STEPS * BR_ZONE_T1_ROLLS land-sampled rolls ahead of tier 2's cheap 24. Sakhal's
    //--- self test went from ~209k to the figure in the current baseline for 200 runs, i.e. a fraction of
    //--- a second on a boot step that runs once. A live match pays it on at most 5 placements.
    //---
    //--- "A healthy generation never leaves tier 1" is unaffected: growth runs only when tier 1 has
    //--- already failed, and ChernarusPlus at stock sizes never gets that far in the first place.
    //---
    //--- THE RESTORE IS THE DANGEROUS PART. Guard (f) has to write r' into s_ChainRadii before it can be
    //--- checked, because CanChainComplete derives reach(level+1) from that entry. So every exit that is
    //--- not a success must put the static value back, or the inflated radius survives into the sweep
    //--- and then the witness step - which TryPlaceLevel accepts with NO FitsWorld check, deliberately,
    //--- on the grounds that it was provably unnecessary. Under an inflated reach it is not, and the
    //--- circle can land partly off the map while CommitChain ships a radius nothing ever validated.
    //--- That is why this is its own method: one place for the restore, and a fresh scope for the
    //--- locals (TryPlaceLevel has already mutated min_pct/max_pct/arc_deg in place by the time it gets
    //--- here, so its tier-1 values are gone, and EnfusionScript allows one declaration per name per
    //--- method scope anyway).
    protected bool TryGrowLevel(int level, vector parent_center, float centre_dir, out vector placed)
    {
        int step;
        int roll;
        float static_radius;
        float static_parent;
        float static_span;
        float grown_radius;
        float grown_span;
        float next_static_span;
        float next_span;
        float increment;
        float min_pct;
        float max_pct;
        float arc_deg;
        float grow_pressure;
        float dist;
        float angle;
        float score;
        vector candidate = "0 0 0";

        placed = "0 0 0";

        if(!m_ZoneSettings || !m_ZoneSettings.allow_zone_size_flex)
            return false;

        //--- (a) Never the seed and never the opening circle. Level 0 is not placed by TryPlaceLevel at
        //--- all, and the opening circle's size is an admin-owned number that Validate() already advises
        //--- on against the map width - growing it behind their back is not this feature's business.
        if(level < 1 || level > (i_NumRounds - 2))
            return false;

        //--- NOTE there is deliberately no pressure gate here - see the header for the two builds that
        //--- established it was refusing every single call. Pressure is still consulted, but only as the
        //--- window bias below, recomputed at the grown radius where it belongs.
        if(!a_StaticSizes || (level + 1) >= a_StaticSizes.Count())
            return false;

        //--- Every exit below the write in (e) has to restore, so bail out here rather than risk a write
        //--- this method cannot undo. Init() fills the array to i_NumRounds and level is at most
        //--- i_NumRounds - 2, so this is unreachable on a sane boot.
        if(!s_ChainRadii || level >= s_ChainRadii.Count())
            return false;

        //--- Growth is measured against the STATIC span, not the current one, so a level that somehow
        //--- arrived here already grown cannot compound it. Reads on their own lines throughout.
        static_radius = a_StaticSizes[level];
        static_parent = a_StaticSizes[level - 1];
        static_span = static_radius - static_parent;

        next_static_span = a_StaticSizes[level + 1] - static_radius;

        if(static_span <= 0 || next_static_span <= 0)
            return false;

        increment = (static_span * BR_ZONE_GROW_MAX_PERCENT) / BR_ZONE_GROW_STEPS;
        if(increment <= 0)
            return false;

        for(step = 1; step <= BR_ZONE_GROW_STEPS; step++)
        {
            grown_radius = static_radius + (increment * step);

            //--- (b) The next level keeps most of its own travel budget. The oracle below covers
            //--- FEASIBILITY; nothing covers QUALITY, and a level left with a fraction of its span sits
            //--- almost on top of its parent, which is the boring chain this whole subsystem exists to
            //--- avoid. At the shipped sizes this never binds.
            next_span = a_StaticSizes[level + 1] - grown_radius;
            if(next_span < (next_static_span * BR_ZONE_GROW_MIN_NEXT_SPAN_PCT))
                break;

            //--- (c) A circle wider than half the world has an empty world-fit box.
            if((2 * grown_radius) > f_WorldSize)
                break;

            //--- (d) The chain's total growth allowance, so a match cannot end up with its whole middle
            //--- quietly fattened.
            if((s_GrowthSpent + (grown_radius - static_radius)) > BR_ZONE_GROW_BUDGET_M)
                break;

            //--- (e) In force from here on, because everything below reads it: GetStepReach for the
            //--- window, CanChainComplete for reach(level+1), GetChainPressure for the bias. THIS IS THE
            //--- WRITE THE RESTORE BELOW EXISTS FOR.
            s_ChainRadii.Set(level, grown_radius);

            grown_span = grown_radius - static_parent;
            grow_pressure = GetChainPressure(parent_center, level);

            //--- The tier-1 window again, recomputed at the grown radius. Same shape as TryPlaceLevel's
            //--- tier 0 - deliberately tier 1 and not a looser one, because the whole point is to get an
            //--- ORDINARY circle where the static radius could only produce a poor one.
            min_pct = DAYZBR_ZS_MIN_DISTANCE_PERCENT;
            max_pct = DAYZBR_ZS_MAX_DISTANCE_PERCENT;
            arc_deg = BR_ZONE_T1_ARC_DEG;

            min_pct = min_pct + ((max_pct - min_pct) * grow_pressure * BR_ZONE_PRESSURE_BIAS);
            arc_deg = arc_deg * (1.0 - (grow_pressure * BR_ZONE_PRESSURE_ARC_TIGHTEN));

            for(roll = 0; roll < BR_ZONE_T1_ROLLS; roll++)
            {
                dist = Math.RandomFloatInclusive(min_pct * grown_span, max_pct * grown_span);
                angle = centre_dir + (Math.RandomFloat(-arc_deg, arc_deg) * Math.DEG2RAD);

                candidate[0] = parent_center[0] + (dist * Math.Sin(angle));
                candidate[2] = parent_center[2] + (dist * Math.Cos(angle));

                //--- Cheapest-first, exactly as in TryPlaceLevel. (f) is the CanChainComplete call: it
                //--- reads the grown radius written above, so it refuses any growth that would strand
                //--- the rest of the chain.
                if(!FitsWorld(candidate, grown_radius))
                    continue;
                if(!CanChainComplete(candidate, level))
                    continue;

                score = GetLandScore(candidate, grown_radius);
                if(score < f_MinLandFraction)
                    continue;

                candidate[1] = GetGame().SurfaceY(candidate[0], candidate[2]);
                placed = candidate;

                s_GrowthSpent = s_GrowthSpent + (grown_radius - static_radius);
                s_GrowthCount = s_GrowthCount + 1;

                BattleRoyaleUtils.Debug("[BattleRoyaleZone] circle " + level + " grew " + static_radius + " -> " + grown_radius + " to place at pressure " + grow_pressure);
                return true;
            }
        }

        //--- THE RESTORE. Every path out of here that is not the success return above lands on this line
        //--- - see the header for what happens if one ever does not.
        s_ChainRadii.Set(level, static_radius);
        return false;
    }

    //--- Place circle `level` around its parent. `desperation` is how many times this level has
    //--- already been attempted against its CURRENT parent; tiers escalate inside one call, and the
    //--- witness step only unlocks once backtracking has had its chance, so a healthy match never
    //--- reaches it.
    protected bool TryPlaceLevel(int level, vector parent_center, int desperation, out vector placed)
    {
        //--- Radii in force, each read on its own line - see ComputeAllowRadii.
        float radius = GetChainRadius(level);
        float parent_radius = GetChainRadius(level - 1);
        float span = radius - parent_radius;
        float centre_dir = Math.Atan2(m_MapCenter[0] - parent_center[0], m_MapCenter[2] - parent_center[2]);
        float pressure = GetChainPressure(parent_center, level);
        float arc_deg = 0;
        float min_pct = 0;
        float max_pct = 0;
        float land_min = 0;
        float dist = 0;
        float angle = 0;
        float score = 0;
        int tier = 0;
        int roll = 0;
        int rolls = 0;
        vector candidate = "0 0 0";

        placed = "0 0 0";

        for(tier = 0; tier < BR_ZONE_TIER_COUNT; tier++)
        {
            //--- Five scalars per tier, chosen with plain branches rather than static const arrays,
            //--- which EnfusionScript will not let a class hold as compile-time constants.
            if(tier == 0)
            {
                arc_deg = BR_ZONE_T1_ARC_DEG;
                min_pct = DAYZBR_ZS_MIN_DISTANCE_PERCENT;
                max_pct = DAYZBR_ZS_MAX_DISTANCE_PERCENT;
                land_min = f_MinLandFraction;
                rolls = BR_ZONE_T1_ROLLS;
            }
            else if(tier == 1)
            {
                arc_deg = BR_ZONE_T2_ARC_DEG;
                min_pct = BR_ZONE_T2_MIN_PCT;
                max_pct = BR_ZONE_T2_MAX_PCT;
                land_min = BR_ZONE_T2_LAND_MIN;
                rolls = BR_ZONE_T2_ROLLS;
            }
            else
            {
                arc_deg = BR_ZONE_T3_ARC_DEG;
                min_pct = BR_ZONE_T3_MIN_PCT;
                max_pct = BR_ZONE_T3_MAX_PCT;
                land_min = BR_ZONE_T3_LAND_MIN;
                rolls = BR_ZONE_T3_ROLLS;
            }

            //--- Adaptive: lift the bottom of the distance window and narrow the cone in proportion
            //--- to how much centre-ward travel this step actually owes. At pressure 0 - the common
            //--- case - both are untouched and the search behaves exactly as it always has.
            min_pct = min_pct + ((max_pct - min_pct) * pressure * BR_ZONE_PRESSURE_BIAS);
            arc_deg = arc_deg * (1.0 - (pressure * BR_ZONE_PRESSURE_ARC_TIGHTEN));

            for(roll = 0; roll < rolls; roll++)
            {
                dist = Math.RandomFloatInclusive(min_pct * span, max_pct * span);
                angle = centre_dir + (Math.RandomFloat(-arc_deg, arc_deg) * Math.DEG2RAD);

                candidate[0] = parent_center[0] + (dist * Math.Sin(angle));
                candidate[2] = parent_center[2] + (dist * Math.Cos(angle));

                //--- Ordered cheapest-first ON PURPOSE. Both tests below are pure arithmetic, and in
                //--- exactly the pathological cases they reject nearly every candidate - so the
                //--- expensive surface sampling is only ever spent on a position that already fits
                //--- the world AND still leaves a completable chain behind it. The old code did the
                //--- opposite: it queried the surface on every roll and only then checked the fit.
                if(!FitsWorld(candidate, radius))
                    continue;
                if(!CanChainComplete(candidate, level))
                    continue;

                score = GetLandScore(candidate, radius);
                if(score < land_min)
                    continue;

                candidate[1] = GetGame().SurfaceY(candidate[0], candidate[2]);
                placed = candidate;
                //--- Get()/Set() on every array that is read AND written in the same scope - see the
                //--- comment in ComputeAllowRadii for what the [] operator does there instead.
                s_TierUsage.Set(tier, s_TierUsage.Get(tier) + 1);
                return true;
            }

            //--- Growth goes HERE - the moment tier 1's preferred window has failed and before tier 2
            //--- loosens the bar. That is #19's condition word for word ("if the 25%/75% can't be
            //--- respected... determine the best between the zone maximum time and the zone size"), and
            //--- it is the only position where growth is a FAIR comparison: it re-runs tier 1's own
            //--- window and tier 1's own land requirement, just at a bigger radius.
            //---
            //--- It was placed after tier 3 first, and the self test proved that cannot work. By then the
            //--- ladder has already accepted 0.10 land over a 180 deg arc and failed even that, so asking
            //--- for f_MinLandFraction (0.60) at a 25% larger radius is a STRICTER bar than the tier that
            //--- just failed - growth measured 0 successes in 1000 placements on Sakhal across two builds,
            //--- once with a pressure gate and once without. Dead code either way, which is the
            //--- BR_ZONE_OFFSET_MIN_DISTANCE = 1500 trap.
            if(tier == 0)
            {
                if(TryGrowLevel(level, parent_center, centre_dir, placed))
                {
                    s_TierUsage.Set(3, s_TierUsage.Get(3) + 1);
                    return true;
                }
            }
        }

        if(SweepPlaceLevel(level, parent_center, centre_dir, placed))
        {
            s_TierUsage.Set(4, s_TierUsage.Get(4) + 1);
            return true;
        }

        //--- Let the caller re-roll the parent first: a different parent is a different annulus, and
        //--- that is far more likely to help than another pass over this one.
        if(desperation < BR_ZONE_LEVEL_RETRIES)
            return false;

        placed = WitnessStep(level, parent_center);
        placed[1] = GetGame().SurfaceY(placed[0], placed[2]);
        s_TierUsage.Set(5, s_TierUsage.Get(5) + 1);

        BattleRoyaleUtils.Warn("[BattleRoyaleZone] circle " + level + " (r=" + radius + ") placed by the fallback step - no land-safe candidate at any tier. The circle is legal but may cover a lot of water.");
        return true;
    }

    //--- Choose where the final circle goes. Three sources, tried in order, each already filtered by
    //--- the oracle so nothing it returns can dead-end later.
    protected bool PickSeedCenter(int seed_attempt, out vector seed)
    {
        float radius;
        float disc;
        float jitter_r;
        float jitter_a;
        float score;
        float best_score = -1;
        int roll;
        int idx;
        int poi_base;
        vector candidate = "0 0 0";
        vector best = "0 0 0";

        seed = "0 0 0";

        if(!a_StaticSizes || a_StaticSizes.Count() == 0)
            return false;

        radius = a_StaticSizes[0];

        //--- (1) A chain-feasible POI.
        //---
        //--- The start of the walk is RANDOM per call, not derived from seed_attempt. Deriving it
        //--- from seed_attempt made the search deterministic: attempt 1 always began at feasible POI
        //--- 0, so the first acceptable village in CfgWorlds order won every single match. Measured
        //--- on Sakhal - 200 self-test generations all seeded identically, 1 seed each, no
        //--- backtracking, which reads like a perfectly healthy result and is actually the tell.
        //--- Walking on from a random start still means a village that fails is followed by a
        //--- different one, which is the part worth keeping.
        if(b_EndInVillages && s_FeasiblePOI && s_FeasiblePOI.Count() > 0)
        {
            poi_base = Math.RandomInt(0, s_FeasiblePOI.Count());

            for(roll = 0; roll < BR_ZONE_SEED_ROLLS; roll++)
            {
                idx = (poi_base + roll) % s_FeasiblePOI.Count();
                array<float> poi = s_FeasiblePOI.Get(idx);
                if(!poi || poi.Count() < 2)
                    continue;

                jitter_r = BR_ZONE_POI_JITTER_M * Math.Sqrt(Math.RandomFloat(0, 1));
                jitter_a = Math.RandomFloat(0, Math.PI2);

                candidate[0] = poi[0] + (jitter_r * Math.Cos(jitter_a));
                candidate[2] = poi[1] + (jitter_r * Math.Sin(jitter_a));

                if(!FitsWorld(candidate, radius))
                    continue;
                if(!CanChainComplete(candidate, 0))
                    continue;
                if(!IsFinalZoneCenterSafe(candidate))
                    continue;

                candidate[1] = GetGame().SurfaceY(candidate[0], candidate[2]);
                seed = candidate;
                return true;
            }
        }

        //--- (2) Random points inside the configured polygon's bounding box. IsFinalZoneCenterSafe
        //--- applies the polygon test itself, so a point outside it is rejected here.
        if(m_ZoneSettings.restrict_final_zone && polygon_vertices && polygon_vertices.Count() >= 3)
        {
            for(roll = 0; roll < BR_ZONE_SEED_ROLLS; roll++)
            {
                candidate[0] = Math.RandomFloat(f_PolyMinX, f_PolyMaxX);
                candidate[2] = Math.RandomFloat(f_PolyMinZ, f_PolyMaxZ);

                if(!FitsWorld(candidate, radius))
                    continue;
                if(!CanChainComplete(candidate, 0))
                    continue;
                if(!IsFinalZoneCenterSafe(candidate))
                    continue;

                candidate[1] = GetGame().SurfaceY(candidate[0], candidate[2]);
                seed = candidate;
                return true;
            }

            BattleRoyaleUtils.Warn("[BattleRoyaleZone] no position inside final_zone_polygon can seed a chain on this map - falling back to the whole feasible disc. restrict_final_zone cannot be honoured here.");
        }

        //--- (3) Uniform sampling of the disc in which EVERY point is provably extendable. This is
        //--- the guarantee that removes the old shutdown path: the disc is non-empty whenever the
        //--- largest circle fits the world at all, which BattleRoyaleZoneData.Validate() enforces.
        //--- sqrt() on the radius makes the sample uniform by area rather than clustered at the
        //--- centre. Best-of rather than first-hit, so an all-water map still yields the driest
        //--- centre available instead of failing.
        disc = GetSeedDiscRadius();
        if(disc >= 0)
        {
            for(roll = 0; roll < BR_ZONE_SEED_ROLLS; roll++)
            {
                jitter_r = disc * Math.Sqrt(Math.RandomFloat(0, 1));
                jitter_a = Math.RandomFloat(0, Math.PI2);

                candidate[0] = m_MapCenter[0] + (jitter_r * Math.Sin(jitter_a));
                candidate[2] = m_MapCenter[2] + (jitter_r * Math.Cos(jitter_a));

                if(!FitsWorld(candidate, radius))
                    continue;
                if(!CanChainComplete(candidate, 0))
                    continue;

                score = GetLandScore(candidate, radius);
                if(score > best_score)
                {
                    best_score = score;
                    best = candidate;
                }

                if(score >= 1.0)
                    break;
            }

            if(best_score >= 0)
            {
                best[1] = GetGame().SurfaceY(best[0], best[2]);
                seed = best;
                return true;
            }
        }

        //--- Unreachable once Validate() has clamped num_zones so the largest circle fits the world.
        BattleRoyaleUtils.Warn("[BattleRoyaleZone] no seed position could be found at all - falling back to the map centre. Check static_sizes against this map's size.");
        seed = m_MapCenter;
        seed[1] = GetGame().SurfaceY(seed[0], seed[2]);
        return true;
    }

    //--- Backtracking search over the whole chain.
    //---
    //--- TERMINATION. Each level's retry counter rises monotonically while its parent is unchanged,
    //--- and at BR_ZONE_LEVEL_RETRIES the level takes the witness step, which is accepted
    //--- unconditionally. So each of the n-1 levels can fail at most BR_ZONE_LEVEL_RETRIES times per
    //--- parent, and re-rolling a parent resets only the levels ABOVE it. Total placements are
    //--- bounded by n * BR_ZONE_LEVEL_RETRIES * BR_ZONE_MAX_SEEDS. This is a proof, not a budget -
    //--- none of those constants is load-bearing for correctness, only for quality.
    protected bool BuildChain(notnull array<vector> chain)
    {
        int n = i_NumRounds;
        int level = 1;
        int seed_attempt = 0;
        int seed_work = 0;
        int k = 0;
        int desperation = 0;
        bool ok = false;
        bool abandon_seed = false;
        vector seed_center = "0 0 0";
        vector placed_center = "0 0 0";
        vector parent_center = "0 0 0";

        chain.Clear();
        s_LevelRetries = new array<int>();

        for(k = 0; k < n; k++)
        {
            chain.Insert("0 0 0");
            s_LevelRetries.Insert(0);
        }

        if(n < 1)
            return false;

        while(seed_attempt < BR_ZONE_MAX_SEEDS)
        {
            seed_attempt++;
            s_SeedsUsed = seed_attempt;
            seed_work = 0;

            if(!PickSeedCenter(seed_attempt, seed_center))
                return false;

            chain.Set(0, seed_center);
            for(k = 1; k < n; k++)
            {
                s_LevelRetries.Set(k, 0);
            }

            //--- A fresh seed is a fresh chain: drop every radius this attempt's predecessor grew, and
            //--- with it the growth budget it spent. Level 0 is never grown, so resetting from 1 keeps
            //--- the loop honest without touching the seed's own circle.
            ResetChainRadiiFrom(1);

            level = 1;

            while(level < n)
            {
                seed_work++;
                s_TotalWork++;

                //--- QUALITY ONLY. Giving up on a seed that keeps costing placements and starting
                //--- again from a different village is the "rewind to the first zone" move. It is
                //--- disabled on the LAST seed attempt, so that attempt always runs to completion
                //--- and the loop can never exit without a chain.
                abandon_seed = false;
                if(seed_work > BR_ZONE_SEED_WORK)
                    abandon_seed = (seed_attempt < BR_ZONE_MAX_SEEDS);

                if(abandon_seed)
                    break;

                //--- Every array read lands in a local BEFORE it is passed to a call - see the long
                //--- comment in ComputeAllowRadii. These are the retry counters that bound the search
                //--- and the chain itself, so a silent misread here corrupts backtracking rather than
                //--- just a log line, and it would look like a tuning problem rather than a bug.
                desperation = s_LevelRetries.Get(level) + 1;
                s_LevelRetries.Set(level, desperation);

                parent_center = chain.Get(level - 1);

                ok = TryPlaceLevel(level, parent_center, desperation, placed_center);
                if(ok)
                {
                    chain.Set(level, placed_center);
                    level++;

                    if(level < n)
                        s_LevelRetries.Set(level, 0);   //--- fresh entry: its parent just changed

                    continue;
                }

                //--- Failed on LAND grounds - geometry can no longer fail, the oracle saw to that.
                //--- Re-roll the parent: its own counter climbs, so it escalates tiers too and at
                //--- BR_ZONE_LEVEL_RETRIES takes its witness step. That is what bounds this loop.
                s_Backtracks++;
                level--;

                if(level > s_DeepestBacktrack)
                    s_DeepestBacktrack = level;

                //--- Never unwind past circle 1. The seed is fixed for this attempt, so re-rolling
                //--- circle 1 is the only thing that can make progress here; reseeding is driven by
                //--- seed_work above, not by unwinding.
                if(level < 1)
                    level = 1;

                //--- Drop the growth of every level from here up. They are no longer placed, so keeping
                //--- their radii would let growth ACCUMULATE across retries - each pass inheriting the
                //--- last one's inflation - and would leave CommitChain shipping a radius for a circle
                //--- whose position was rolled again afterwards. The budget is refunded with it.
                ResetChainRadiiFrom(level);
            }

            if(level >= n)
                return true;
        }

        return false;
    }

    //--- Turn a finished chain into the play areas, and work out the round timing from it.
    //---
    //--- EVERY TIMING TERM IS COMPUTED HERE, FROM THE FINISHED CHAIN, AND NEVER DURING PLACEMENT. The
    //--- original reason was backtracking: a single scratch slot cannot survive it, because a re-rolled
    //--- level's offset has to be discarded and the old code had no way to do that. Radius flex added a
    //--- second and sharper reason. s_PlayAreaDurationOffsets is allocated only in this method, while
    //--- RunSelfTest calls BuildChain 50-200 times AFTER a real chain has already been committed and
    //--- deliberately never re-commits - so an accumulator inside TryGrowLevel or TryPlaceLevel would
    //--- fold hundreds of throwaway chains' payments into the array the live match is about to read, and
    //--- pin every entry at BR_ZONE_OFFSET_MAX_SECONDS. Anybody following the documented acceptance gate
    //--- (zone_selftest_runs > 0) would silently get +120 s on every round of every match.
    //---
    //--- So: no `+=` anywhere in here or upstream of it. Growth is recovered by DIFFING the radii that
    //--- survived against the admin's static sizes, which is state that cannot be corrupted by a
    //--- throwaway run because a throwaway run never reaches this method.
    protected void CommitChain(notnull array<vector> chain)
    {
        int i;
        float dist;
        float offset;
        float radius;
        float static_radius;
        float grown;
        float distance_term;
        float growth_term;
        float travel;
        float derived;
        float parent_radius;
        float opening;
        vector centre;
        vector from_2d;
        vector to_2d;

        m_PlayAreas = new array<ref BattleRoyalePlayArea>();
        s_PlayAreaDurationOffsets = new array<float>();
        s_PlayAreaDerivedTimers = new array<float>();
        s_PlayAreaOpeningTimers = new array<float>();

        for(i = 0; i < chain.Count(); i++)
        {
            //--- Every read into a local before the constructor call, as everywhere else here. The radius
            //--- is the one IN FORCE, so a grown circle is committed at the size it was validated at -
            //--- and because every consumer outside this file reads radii through
            //--- BattleRoyalePlayArea.GetRadius(), that is the whole of what makes flex reach the states,
            //--- the RPCs and the client.
            centre = chain.Get(i);
            radius = GetChainRadius(i);

            m_PlayAreas.Insert(new BattleRoyalePlayArea(centre, radius));
            s_PlayAreaDurationOffsets.Insert(0);
            s_PlayAreaDerivedTimers.Insert(0);

            //--- The opening-round price for THIS circle, computed for every index because which one
            //--- is the opening circle depends on the player count and is not known until the
            //--- countdown. Loot allowance plus the sprint a spawned player still owes inside the
            //--- circle; no inbound travel, because the opening round is the one round players start
            //--- already inside its circle. Divided by the lock fraction for the same reason the
            //--- derived timer below is: the circle bites at 80% of the round, so a travel term has to
            //--- fit in that 80%.
            opening = BR_ZONE_TIMER_LOOT_SECONDS + (((radius * BR_ZONE_TIMER_OPENING_SPREAD) / BR_ZONE_TIMER_SPEED_MPS) / BR_ZONE_LOCK_FRACTION);

            if(opening < BR_ZONE_TIMER_MIN_SECONDS)
                opening = BR_ZONE_TIMER_MIN_SECONDS;
            if(opening > BR_ZONE_TIMER_MAX_SECONDS)
                opening = BR_ZONE_TIMER_MAX_SECONDS;

            s_PlayAreaOpeningTimers.Insert(opening);
        }

        //--- The travel between circle i and circle i-1 belongs to the round that moves players INTO
        //--- circle i-1, i.e. play area index i-1. Measured in 2D: the old code compared positions
        //--- whose Y came from SurfaceY, so hilly terrain inflated the distance.
        //---
        //--- GROWTH IS CREDITED TO INDEX i-1 FOR THE SAME REASON, and getting that backwards is the
        //--- tempting mistake. During the round whose settings index is j, players are inside circle j+1
        //--- and must reach circle j - which is exactly what 6_BattleRoyaleRound.Activate sends as
        //--- UpdateCurrentPlayArea and UpdateFuturePlayArea respectively. Worst-case travel is therefore
        //--- |c_j+1 - c_j| + r_j+1 - r_j, so growing r_L makes round L-1 HARDER and round L slightly
        //--- EASIER. Paying index L would pay the round that got easier and starve the one that did not.
        for(i = 1; i < chain.Count(); i++)
        {
            from_2d = chain.Get(i);
            to_2d = chain.Get(i - 1);
            from_2d[1] = 0;
            to_2d[1] = 0;

            dist = vector.Distance(from_2d, to_2d);

            //--- The distance term keeps its threshold: a short hop needs no bonus at all. But the
            //--- threshold must NOT skip the whole iteration the way it used to, or the growth term goes
            //--- with it - and levels 1-2, whose spans are 105-422 m and can never cross 600 m, are
            //--- precisely where growth fires most.
            distance_term = 0;
            if(dist > BR_ZONE_OFFSET_MIN_DISTANCE)
                distance_term = dist / BR_ZONE_OFFSET_SPEED_MPS;

            //--- Extra ground inside circle i, which the round playing it has to cross to leave it.
            static_radius = 0;
            if(a_StaticSizes && i < a_StaticSizes.Count())
                static_radius = a_StaticSizes[i];

            grown = GetChainRadius(i);

            growth_term = 0;
            if(grown > static_radius)
                growth_term = ((grown - static_radius) / BR_ZONE_OFFSET_SPEED_MPS) / BR_ZONE_LOCK_FRACTION;

            offset = distance_term + growth_term;

            //--- Capped once, after both terms, so the two cannot each spend the whole allowance.
            if(offset > BR_ZONE_OFFSET_MAX_SECONDS)
                offset = BR_ZONE_OFFSET_MAX_SECONDS;

            if(offset > 0)
                s_PlayAreaDurationOffsets.Set(i - 1, offset);

            //--- The derived timer for that same round, whether or not it is switched on - computing it
            //--- unconditionally costs nothing and means the diag table can show what turning the setting
            //--- on WOULD do. Radii in force, so growth is priced structurally here rather than added on
            //--- top; that is why GetZoneTimer's derive branch deliberately skips GetDurationOffset.
            parent_radius = GetChainRadius(i - 1);

            travel = dist + grown - parent_radius;
            derived = ((travel / BR_ZONE_TIMER_SPEED_MPS) / BR_ZONE_LOCK_FRACTION) + BR_ZONE_TIMER_FIGHT_SECONDS;

            if(derived < BR_ZONE_TIMER_MIN_SECONDS)
                derived = BR_ZONE_TIMER_MIN_SECONDS;

            if(derived > BR_ZONE_TIMER_MAX_SECONDS)
            {
                derived = BR_ZONE_TIMER_MAX_SECONDS;

                //--- Only worth saying when the value is actually in use. A bound clamp quietly
                //--- reintroduces the under-time problem the derivation exists to remove, so it must not
                //--- pass silently - but warning about it on a server that never turned the setting on
                //--- would just be noise about a number nothing reads.
                if(IsDerivedTimingActive())
                    BattleRoyaleUtils.Warn("[BattleRoyaleZone] the derived timer for circle " + (i - 1) + " hit the " + BR_ZONE_TIMER_MAX_SECONDS + " s cap - that round is shorter than its own geometry asks for. Raise BR_ZONE_TIMER_MAX_SECONDS or lower this static_sizes span.");
            }

            s_PlayAreaDerivedTimers.Set(i - 1, derived);
        }
    }

    //=====================================================================================
    //--- DERIVED LADDER (#284 point 1). How many players each PLACED circle is rated for, from its
    //--- radius and from how much loot it actually encloses.
    //---
    //--- Run once per process from GenerateAll, immediately after CommitChain, and UNCONDITIONALLY -
    //--- same reasoning as s_PlayAreaDerivedTimers: computing it costs n x |s_POI| distance tests
    //--- (~1700 on ChernarusPlus, pure arithmetic, no native calls) and it means the boot report and
    //--- the admin zone table can show what turning derive_zone_ladder on WOULD do, which is precisely
    //--- when an operator wants to know.
    //---
    //--- NOT in CommitChain, for two reasons: these are not timing terms, so the "every timing term is
    //--- computed here" rule does not want them; and s_POI is only guaranteed populated after
    //--- BuildFeasiblePOIList, which GenerateAll calls and CommitChain has no business depending on.
    //=====================================================================================

    //--- POIs whose position falls inside this circle. s_POI is already avoid-list filtered and
    //--- resolved onto real buildings, so it counts "places worth looting" rather than "labels in
    //--- CfgWorlds". Squared distance, so no square root and no native call.
    protected int CountPOIsInCircle(vector centre, float radius)
    {
        int i;
        int hits = 0;
        float dx;
        float dz;
        float px;
        float pz;
        float r2 = radius * radius;
        array<float> poi;

        if(!s_POI || radius <= 0)
            return 0;

        for(i = 0; i < s_POI.Count(); i++)
        {
            //--- Element into a local before anything reads it, per ComputeAllowRadii's rule.
            poi = s_POI.Get(i);
            if(!poi)
                continue;
            if(poi.Count() < 2)
                continue;

            //--- s_POI holds 2-element [x, z] pairs - poi[1] is Z, not Y.
            px = poi[0];
            pz = poi[1];

            dx = px - centre[0];
            dz = pz - centre[2];

            if(((dx * dx) + (dz * dz)) <= r2)
                hits++;
        }

        return hits;
    }

    /**
     *  Lootable buildings inside each placed circle, from the cached map-wide census.
     *
     *  This replaced a per-circle native scan that cost 8.7 s on EVERY boot (167 probes over one
     *  3375 m circle, measured on ChernarusPlus) and could only ever see relative poverty, because
     *  with no map-wide figure the reference density had to be the largest circle's own - pinning that
     *  circle at factor 1.0 and rating opening circles of 1316, 2744 and 3174 buildings identically.
     *
     *  BattleRoyaleBuildingCensus scans the world once, caches it in scan_cache.json, and hands
     *  back exact counts for any circle. See its header for why the scan is a lattice of small probes
     *  and why each building is counted exactly once.
     */
    protected void CensusBuildings()
    {
        int i;
        int count;
        float radius;
        vector centre;
        BattleRoyalePlayArea area;

        s_PlayAreaBuildings = new array<int>();
        s_CensusCells = 0;
        s_CensusFound = 0;
        s_CensusMs = 0;

        if (!m_PlayAreas || m_PlayAreas.Count() == 0)
            return;

        if (!BattleRoyaleBuildingCensus.IsReady())
        {
            //--- No census: BuildDerivedLadder falls back to POI counts and says so in the boot report.
            s_PlayAreaBuildings = NULL;
            return;
        }

        for (i = 0; i < m_PlayAreas.Count(); i++)
        {
            area = m_PlayAreas.Get(i);
            if (!area)
            {
                s_PlayAreaBuildings.Insert(0);
                continue;
            }

            centre = area.GetCenter();
            radius = area.GetRadius();

            count = BattleRoyaleBuildingCensus.CountInCircle(centre, radius);
            s_PlayAreaBuildings.Insert(count);
        }

        s_CensusFound = BattleRoyaleBuildingCensus.GetBuildingCount();
    }

    //--- Lootable buildings inside this circle, or -1 when no census ran.
    static int GetBuildingCount(int play_area_index)
    {
        if (!s_PlayAreaBuildings)
            return -1;

        if (play_area_index < 0 || play_area_index >= s_PlayAreaBuildings.Count())
            return -1;

        return s_PlayAreaBuildings.Get(play_area_index);
    }

    protected void BuildDerivedLadder()
    {
        int i;
        int poi_count;
        int density_count;
        int derived_min;
        int floor_players;
        float radius;
        float area_m2;
        float circle_density;
        float map_density;
        float world_area;
        float ratio;
        float factor;
        float metres_per_player;
        float factor_min;
        float factor_max;
        bool use_buildings;
        vector centre;
        BattleRoyalePlayArea area;

        s_PlayAreaPOICount = new array<int>();
        s_PlayAreaMinPlayers = new array<int>();

        if(!m_PlayAreas)
            return;

        //--- Because this runs whether or not the feature is on, the knobs have not necessarily been
        //--- through BattleRoyaleZoneData.Validate's clamp - that block is gated on derive_zone_ladder.
        //--- Bound them here too rather than dividing by a hand-edited zero.
        metres_per_player = Math.Clamp(f_MetresPerPlayer, BR_ZONE_LADDER_MIN_M_PER_PLAYER, BR_ZONE_LADDER_MAX_M_PER_PLAYER);

        factor_min = f_LootFactorMin;
        factor_max = f_LootFactorMax;
        if(factor_min <= 0 || factor_max < factor_min)
        {
            factor_min = 0.5;
            factor_max = 1.5;
        }

        floor_players = i_MinPlayersFloor;
        if(floor_players < 1)
            floor_players = 1;

        //--- THE YARDSTICK: the whole map's building density. Buildings when the census is available,
        //--- POI counts only as a fallback.
        //---
        //--- ⚠️ IT IS MAP-WIDE AND THAT IS THE POINT, not a detail. An earlier build used the largest
        //--- circle's own density as the reference, which pins that circle at factor 1.0 by
        //--- construction - so a chain landing in a uniformly loot-poor region rated exactly like one
        //--- in a rich region, and opening circles holding 1316, 2744 and 3174 buildings were all
        //--- rated for 33 players. Against an absolute reference a poor region genuinely lowers every
        //--- rating, the selection walk stops at a BIGGER circle, and the match gains a shrink - which
        //--- is the behaviour #284 asked for and the relative reference could not express.
        world_area = f_WorldSize * f_WorldSize;
        map_density = 0;
        use_buildings = false;

        if(BattleRoyaleBuildingCensus.IsReady() && s_PlayAreaBuildings)
        {
            map_density = BattleRoyaleBuildingCensus.GetMapDensity();
            use_buildings = (map_density > 0);
        }

        if(!use_buildings && world_area > 0 && s_POI)
            map_density = s_POI.Count() / world_area;

        for(i = 0; i < m_PlayAreas.Count(); i++)
        {
            area = m_PlayAreas.Get(i);
            if(!area)
            {
                s_PlayAreaPOICount.Insert(0);
                s_PlayAreaMinPlayers.Insert(floor_players);
                continue;
            }

            centre = area.GetCenter();
            radius = area.GetRadius();

            poi_count = CountPOIsInCircle(centre, radius);
            s_PlayAreaPOICount.Insert(poi_count);

            //--- Buildings when we have them, POIs only when the census could not run.
            density_count = poi_count;
            if(use_buildings)
                density_count = GetBuildingCount(i);

            //--- ⚠️ THE DIRECTION READS BACKWARDS AND IS RIGHT. A circle DENSER than the map average
            //--- gets a HIGHER min_players, which makes GetDynamicStartingZone's walk pass over it and
            //--- settle on a SMALLER opening circle. That is the wanted behaviour: a loot-rich region
            //--- feeds the same crowd on less ground. The instinct to make a rich circle "support more
            //--- players" and therefore be picked more readily has the selection walk backwards - it
            //--- takes the FIRST circle whose rating is below the population, largest first.
            //---
            //--- A map with no census and no POIs at all leaves the factor at 1, i.e. radius alone, rather
            //--- collapsing every circle onto the floor.
            factor = 1.0;
            area_m2 = Math.PI * radius * radius;

            if(map_density > 0 && area_m2 > 0)
            {
                circle_density = density_count / area_m2;
                ratio = circle_density / map_density;
                factor = 1.0 + ((ratio - 1.0) * f_LootDensityWeight);
            }

            factor = Math.Clamp(factor, factor_min, factor_max);

            //--- Linear in RADIUS, not in area, and that is measured rather than chosen: the shipped
            //--- min_players table works out to 3375/33 = 2250/22 = 1125/11 = 102.3 m per player, with
            //--- the four smaller tiers on a floor of 10. So the RADIUS TERM ALONE - i.e.
            //--- zone_loot_density_weight = 0 - reproduces {10, 10, 10, 11, 22, 33} exactly, and that is
            //--- the neutrality check to run when tuning this: set the weight to 0 and the derived
            //--- column in the boot report must match zone_settings.json entry for entry.
            //---
            //--- ⚠️ AT THE DEFAULT WEIGHT OF 1 IT DOES NOT MATCH, AND THAT IS EXPECTED, NOT A BUG.
            //--- Every circle in the chain is nested around a village seed (end_in_villages), so every
            //--- circle is denser than the map mean and the factor is above 1 for all of them - most at
            //--- the small end, where the circle is a town and the map is a country. Two consequences
            //--- worth knowing before tuning: that part of the factor is a smooth function of radius
            //--- rather than a statement about this match, so it is absorbed by lowering
            //--- zone_metres_per_player rather than by fighting the weight; and what is left AFTER it -
            //--- the run-to-run difference between a chain that landed in a dense part of the map and
            //--- one that did not - is the real signal, and is what #284 asked for.
            derived_min = Math.Round((radius / metres_per_player) * factor);
            if(derived_min < floor_players)
                derived_min = floor_players;

            s_PlayAreaMinPlayers.Insert(derived_min);
        }
    }

    //--- One row per circle, in ARRAY order like LogGeneratedChain (index 0 is the FINAL circle).
    //--- Built in steps: a row carries six fields and a single concatenated expression past about ten
    //--- terms is "Formula too complex", a hard compile error that packing does not catch.
    protected void LogDerivedLadder()
    {
        int i;
        int poi_count;
        int derived_min;
        int authored_min;
        string head;
        string row;
        BattleRoyalePlayArea area;

        if(!m_PlayAreas || !s_PlayAreaMinPlayers)
            return;

        head = "[BattleRoyaleZone] derived ladder: derive_zone_ladder=";
        head = head + b_DeriveLadder;
        head = head + " metres_per_player=" + f_MetresPerPlayer;
        head = head + " floor=" + i_MinPlayersFloor;
        head = head + " density_weight=" + f_LootDensityWeight;
        //--- Which input the density term actually used. Without this the derived column is ambiguous
        //--- in the one direction that matters: a silent fallback to POI counts looks like a working
        //--- census, and POI counts are the thing this replaced.
        if(s_PlayAreaBuildings)
            head = head + " density_from=BUILDINGS";
        else
            head = head + " density_from=POIs(fallback)";
        BattleRoyaleUtils.Info(head);

        for(i = 0; i < m_PlayAreas.Count(); i++)
        {
            //--- Every read into a local on its own line before it is used - see ComputeAllowRadii.
            area = m_PlayAreas.Get(i);
            if(!area)
                continue;

            poi_count = GetPOICount(i);
            derived_min = GetDerivedMinPlayers(i);

            authored_min = -1;
            if(a_MinPlayers && i < a_MinPlayers.Count())
                authored_min = a_MinPlayers[i];

            row = "[BattleRoyaleZone]   [" + i + "] radius=" + area.GetRadius();
            row = row + " buildings=" + GetBuildingCount(i);
            row = row + " pois=" + poi_count;
            row = row + " min_players derived=" + derived_min;
            row = row + " authored=" + authored_min;
            row = row + " opening_timer=" + GetOpeningTimer(i);

            BattleRoyaleUtils.Info(row);
        }
    }

    protected void LogGeneratedChain()
    {
        int i;
        int surface_calls = s_SurfaceCalls;
        //--- Built up in steps rather than concatenated in one go. A single expression has a complexity
        //--- ceiling in EnfusionScript - around ten terms - and "Formula too complex" is a hard compile
        //--- error that packing does not catch, so it only surfaces when the game loads the module. The
        //--- tier line was already at eleven terms before growth added a sixth column.
        string tiers;
        string row;
        float growth;
        BattleRoyalePlayArea area;

        BattleRoyaleUtils.Info("[BattleRoyaleZone] generated " + m_PlayAreas.Count() + " circles: " + s_TotalWork + " placements, " + s_Backtracks + " backtracks (deepest level " + s_DeepestBacktrack + "), " + s_SeedsUsed + " seed(s), " + surface_calls + " surface queries.");

        tiers = "[BattleRoyaleZone] tier usage: T1 " + s_TierUsage.Get(0);
        tiers = tiers + "  T2 " + s_TierUsage.Get(1);
        tiers = tiers + "  T3 " + s_TierUsage.Get(2);
        tiers = tiers + "  growth " + s_TierUsage.Get(3);
        tiers = tiers + "  sweep " + s_TierUsage.Get(4);
        tiers = tiers + "  fallback-step " + s_TierUsage.Get(5) + ".";
        BattleRoyaleUtils.Info(tiers);

        if(s_GrowthCount > 0)
            BattleRoyaleUtils.Info("[BattleRoyaleZone] radius flex: " + s_GrowthCount + " circle(s) grew, " + s_GrowthSpent + " m of the " + BR_ZONE_GROW_BUDGET_M + " m budget spent.");

        //--- Index 0 is the FINAL circle - the array is smallest-first, like the settings arrays.
        for(i = 0; i < m_PlayAreas.Count(); i++)
        {
            //--- Element into a local first, and one array read per statement. The old one-liner read
            //--- m_PlayAreas twice and s_PlayAreaDurationOffsets once inside a single expression full of
            //--- calls - the exact shape ComputeAllowRadii documents as silently returning another
            //--- array's entries. It was a log line, so a misread would have been believed.
            area = m_PlayAreas.Get(i);
            growth = GetRadiusGrowth(i);

            row = "[BattleRoyaleZone]   [" + i + "] center=" + area.GetCenter();
            row = row + " radius=" + area.GetRadius();
            if(growth > 0)
                row = row + " (grew +" + growth + ")";
            row = row + " duration_offset=" + s_PlayAreaDurationOffsets.Get(i);
            row = row + " derived_timer=" + s_PlayAreaDerivedTimers.Get(i);

            BattleRoyaleUtils.Debug(row);
        }
    }

    //--- Generate `iterations` throwaway chains and report the distribution. Nothing it does touches
    //--- m_PlayAreas - BuildChain only fills the array handed to it, and CommitChain is not called -
    //--- so it is safe to run after the real circles are already committed.
    //---
    //--- This exists because a generation that dead-ends was previously unreproducible: the process
    //--- restarts between matches, placement is random, and a bad layout is a coin flip. Twenty
    //--- relaunches could not establish what 200 runs inside one boot establish directly.
    void RunSelfTest(int iterations)
    {
        array<vector> chain = new array<vector>();
        array<int> depth_hist = new array<int>();
        array<int> seed_hist = new array<int>();
        array<int> tier_total = new array<int>();
        int completed = 0;
        int failed = 0;
        int run;
        int k;
        int depth;
        int seeds;
        int started_ms = GetGame().GetTime();
        int surface_before = s_SurfaceCalls;
        int work_total = 0;
        int capped = 0;
        string line;
        //--- Spread of the FINAL circle across runs. Without this the test cannot tell a healthy
        //--- generator from one that returns the same answer every time - which is exactly the bug
        //--- it missed once already, reporting 200/200 with no backtracking while every single run
        //--- seeded on the same village.
        float seed_min_x = float.MAX;
        float seed_max_x = float.LOWEST;
        float seed_min_z = float.MAX;
        float seed_max_z = float.LOWEST;
        vector first_circle;
        //--- How much radius flex the throwaway runs asked for, so the acceptance gate can report it as
        //--- its own column instead of it only being visible on the one live chain.
        int grown_runs = 0;
        float grown_metres = 0;
        //--- Scratch for the two array reads below that would otherwise share an expression with a call.
        float snapshot_radius;
        int tier_run;
        //--- The committed chain's radii, put back before this method returns, because BuildChain leaves
        //--- s_ChainRadii holding whatever the LAST throwaway run happened to grow.
        //---
        //--- Two other things already stop that mattering, and this is a third line of defence rather
        //--- than the load-bearing one: Init() calls ResetChainRadii() immediately before
        //--- ComputeAllowRadii(), so a zone object built after this point derives a_AllowRadius from the
        //--- static sizes either way; and GetRadiusGrowth reads the committed play area rather than this
        //--- array. What it buys is that s_ChainRadii is never left describing a chain nobody plays -
        //--- same intent as the RNG reseed at the bottom of this method, and cheap enough to just do.
        array<float> committed_radii = new array<float>();

        if(iterations < 1)
            return;

        if(s_ChainRadii)
        {
            for(k = 0; k < s_ChainRadii.Count(); k++)
            {
                //--- Read into a local before the Insert, rather than
                //---     committed_radii.Insert(s_ChainRadii.Get(k));
                //--- which is the two-arrays-in-one-expression shape ComputeAllowRadii documents.
                snapshot_radius = s_ChainRadii.Get(k);
                committed_radii.Insert(snapshot_radius);
            }
        }

        for(k = 0; k <= i_NumRounds; k++)
        {
            depth_hist.Insert(0);
        }
        for(k = 0; k <= BR_ZONE_MAX_SEEDS; k++)
        {
            seed_hist.Insert(0);
        }
        for(k = 0; k < BR_ZONE_TIER_SLOTS; k++)
        {
            tier_total.Insert(0);
        }

        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest] running " + iterations + " generations on " + GetGame().GetWorldName() + "...");

        for(run = 0; run < iterations; run++)
        {
            ResetGenerationStats();

            if(BuildChain(chain))
                completed++;
            else
                failed++;

            //--- Every read lands in a local before it is used, per the note in ComputeAllowRadii.
            depth = s_DeepestBacktrack;
            seeds = s_SeedsUsed;
            work_total = work_total + s_TotalWork;

            if(chain.Count() > 0)
            {
                first_circle = chain.Get(0);
                seed_min_x = Math.Min(seed_min_x, first_circle[0]);
                seed_max_x = Math.Max(seed_max_x, first_circle[0]);
                seed_min_z = Math.Min(seed_min_z, first_circle[2]);
                seed_max_z = Math.Max(seed_max_z, first_circle[2]);
            }

            if(s_GrowthCount > 0)
            {
                grown_runs++;
                grown_metres = grown_metres + s_GrowthSpent;
            }

            if(depth >= 0 && depth < depth_hist.Count())
                depth_hist.Set(depth, depth_hist.Get(depth) + 1);
            if(seeds >= 0 && seeds < seed_hist.Count())
                seed_hist.Set(seeds, seed_hist.Get(seeds) + 1);

            for(k = 0; k < BR_ZONE_TIER_SLOTS; k++)
            {
                //--- Two DIFFERENT arrays were being read inside one Set() call here. That is the shape
                //--- ComputeAllowRadii documents as silently yielding another array's entries, and this
                //--- is the acceptance gate's own tally - a misread here would be believed.
                tier_run = s_TierUsage.Get(k);
                tier_total.Set(k, tier_total.Get(k) + tier_run);
            }

            if(work_total > BR_ZONE_SELFTEST_WORK_CAP)
            {
                capped = 1;
                break;
            }
        }

        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest] " + (completed + failed) + " runs, " + (GetGame().GetTime() - started_ms) + " ms, " + (s_SurfaceCalls - surface_before) + " surface calls, " + work_total + " placements.");
        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest]   completed " + completed + ", HARD FAILURES " + failed + ".");

        line = "";
        for(k = 0; k < depth_hist.Count(); k++)
        {
            if(depth_hist.Get(k) > 0)
                line = line + " depth" + k + "=" + depth_hist.Get(k);
        }
        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest]   backtrack depth:" + line);

        line = "";
        for(k = 0; k < seed_hist.Count(); k++)
        {
            if(seed_hist.Get(k) > 0)
                line = line + " " + k + "seed(s)=" + seed_hist.Get(k);
        }
        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest]   seeds used:" + line);

        //--- Built in steps: six columns is past the expression complexity ceiling, and "Formula too
        //--- complex" is a compile error that only shows up when the game loads the module.
        line = "[BattleRoyaleZone][SelfTest]   tier per circle: T1 " + tier_total.Get(0);
        line = line + "  T2 " + tier_total.Get(1);
        line = line + "  T3 " + tier_total.Get(2);
        line = line + "  growth " + tier_total.Get(3);
        line = line + "  sweep " + tier_total.Get(4);
        line = line + "  fallback-step " + tier_total.Get(5) + ".";
        BattleRoyaleUtils.Info(line);

        //--- Reported unconditionally, including the zero, because "flex never fired" is exactly what a
        //--- run with allow_zone_size_flex off has to be able to say for the comparison to mean anything.
        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest]   radius flex: " + grown_runs + " run(s) grew a circle, " + grown_metres + " m total.");

        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest]   final circle spread: x[" + seed_min_x + " .. " + seed_max_x + "] z[" + seed_min_z + " .. " + seed_max_z + "].");

        if((seed_max_x - seed_min_x) < 1.0 && (seed_max_z - seed_min_z) < 1.0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][SelfTest] every run produced the SAME final circle - generation is not varying between matches. Expect the same endgame location every time.");

        if(capped > 0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][SelfTest] stopped early at the work cap (" + BR_ZONE_SELFTEST_WORK_CAP + " placements) - this map is costing far more search than a healthy one.");

        if(failed > 0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][SelfTest] " + failed + " run(s) produced NO chain at all. That should be impossible - check static_sizes against this map's size.");

        //--- Put the committed chain's radii back - see committed_radii's declaration. Same intent as the
        //--- RNG line below: this method must leave no trace on the match about to be played.
        if(s_ChainRadii)
        {
            for(k = 0; k < committed_radii.Count(); k++)
            {
                if(k < s_ChainRadii.Count())
                    s_ChainRadii.Set(k, committed_radii.Get(k));
            }
        }

        //--- Leave the RNG somewhere fresh so the match about to be played is not correlated with the
        //--- stream this test just consumed.
        Math.Randomize(Math.RandomInt(1, 2000000000));
    }

    /**
     *  Walk player counts 1..max_players and report which circle each one would open on (#284).
     *
     *  The acceptance gate for derive_zone_ladder and bound_match_duration, and it exists for exactly
     *  the reason zone_selftest_runs does: the interesting cases are combinatorial in a variable
     *  (the player count) that LaunchLocalMP.bat can only ever set to three.
     *
     *  ⚠️ THE TIER HISTOGRAM IS THE POINT, not the rows. This repo has twice shipped a derivation that
     *  compiled, ran, and never once changed the answer - BR_ZONE_OFFSET_MIN_DISTANCE at 1500 against
     *  a longest possible step under 1000, and TryGrowLevel sitting after a tier that had already
     *  accepted a looser bar. Both were caught by a counter, and neither was visible from the code.
     *  A histogram with one populated bucket means the ladder is dead code however good the formula is.
     *
     *  Touches nothing the match reads: it asks the same questions the countdown will ask and then puts
     *  the memo back, the same discipline RunSelfTest applies to s_ChainRadii.
     */
    void RunLadderSelfTest(int max_players)
    {
        array<int> tier_hist = new array<int>();
        int p;
        int k;
        int zone_number;
        int settings_index;
        int last_zone = -1;
        int distinct = 0;
        int poi_count;
        int duration;
        int bucket;
        int moved = 0;
        int span_seconds;
        int shortest = 0;
        int longest = 0;
        int floor_zone;
        float radius;
        float per_poi;
        string line;
        BattleRoyaleZone zone;
        BattleRoyalePlayArea area;

        if(max_players < 1)
            return;

        for(k = 0; k <= i_NumRounds; k++)
        {
            tier_hist.Insert(0);
        }

        BattleRoyaleUtils.Info("[BattleRoyaleZone][LadderTest] walking 1.." + max_players + " players on " + GetGame().GetWorldName() + "...");

        //--- What this ladder can actually produce, before anything is asked of it. Reported first
        //--- because it is the yardstick every duration setting has to be chosen against, and because
        //--- an operator who sets match_max_seconds below `longest` has made the biggest circle
        //--- unreachable at every population - which looks like a working feature and is not.
        //---
        //--- ⚠️ THE RANGE IS OVER LEGAL STARTING TIERS, i.e. 1..floor_zone, NOT 1..num_zones. The walk
        //--- can never start past floor_zone (min_zone_num is an explicit "play at least this many
        //--- circles"), so pricing zone num_zones reports a match nobody can ever be given. Written
        //--- that way first, and the warning below then fired against a 394 s "shortest" that the very
        //--- next line contradicted with 1242 s - advising the operator to fix a setting that was fine.
        floor_zone = Math.Max(1, i_NumRounds - m_ZoneSettings.min_zone_num + 1);

        for(k = 1; k <= floor_zone; k++)
        {
            span_seconds = BattleRoyaleState.GetMatchDurationSeconds(k);
            if(k == 1)
                longest = span_seconds;

            shortest = span_seconds;
        }

        line = "[BattleRoyaleZone][LadderTest]   this ladder can play " + shortest + " s (zone " + floor_zone + ")";
        line = line + " to " + longest + " s (zone 1),";
        line = line + " i.e. starting tiers 1.." + floor_zone + " at min_zone_num " + m_ZoneSettings.min_zone_num + ".";
        BattleRoyaleUtils.Info(line);

        for(p = 1; p <= max_players; p++)
        {
            zone_number = BattleRoyaleState.GetDynamicStartingZone(p);

            //--- ON THE VERY NEXT LINE, and nowhere else - s_LastBoundMove is a return value in all
            //--- but name. It is what makes "the duration bound is quietly deciding this on its own"
            //--- visible: the chosen tier alone cannot show it, because a hijacked answer looks exactly
            //--- like a legitimate one.
            if(BattleRoyaleState.s_LastBoundMove != 0)
                moved++;

            //--- Read into a local before the Set(), per ComputeAllowRadii's rule. This is the gate's
            //--- own tally; a misread here would be believed.
            if(zone_number >= 0 && zone_number < tier_hist.Count())
            {
                bucket = tier_hist.Get(zone_number);
                tier_hist.Set(zone_number, bucket + 1);
            }

            //--- One row per CHANGE rather than one per player count: the answer is a step function and
            //--- printing every step of a flat stretch would hide the steps that matter.
            if(zone_number == last_zone)
                continue;

            last_zone = zone_number;
            distinct++;

            settings_index = i_NumRounds - zone_number;

            radius = 0;
            zone = BattleRoyaleZone.GetZone(zone_number);
            if(zone)
            {
                area = zone.GetArea();
                if(area)
                    radius = area.GetRadius();
            }

            poi_count = GetPOICount(settings_index);
            duration = BattleRoyaleState.GetMatchDurationSeconds(zone_number);

            per_poi = 0;
            if(poi_count > 0)
                per_poi = p / (poi_count * 1.0);

            line = "[BattleRoyaleZone][LadderTest]   " + p + "+ players -> zone " + zone_number;
            line = line + " (index " + settings_index + ")";
            line = line + " r=" + radius;
            line = line + " pois=" + poi_count;
            line = line + " players/poi=" + per_poi;
            line = line + " match=" + duration + " s";
            BattleRoyaleUtils.Info(line);
        }

        line = "";
        for(k = 0; k < tier_hist.Count(); k++)
        {
            bucket = tier_hist.Get(k);
            if(bucket > 0)
                line = line + " zone" + k + "=" + bucket;
        }
        BattleRoyaleUtils.Info("[BattleRoyaleZone][LadderTest]   tier histogram:" + line);
        BattleRoyaleUtils.Info("[BattleRoyaleZone][LadderTest]   duration bound moved the tier for " + moved + " of " + max_players + " player counts.");

        if(distinct <= 1)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][LadderTest] every player count from 1 to " + max_players + " opens on the SAME circle - the ladder is not adapting to anything. Check zone_metres_per_player against static_sizes, and min_zone_num against num_zones.");

        //--- ⚠️ THE ONE THE HISTOGRAM CANNOT SHOW. A bound that overrides most of the walk still leaves
        //--- several populated buckets - it has simply replaced the player-count and loot answer with a
        //--- clock, and every tier boundary you are looking at is the clock's. Measured on ChernarusPlus
        //--- during #284's own acceptance run: 100 of 100 moved, and the largest circle was unreachable
        //--- at every population, while the histogram looked perfectly healthy at two buckets.
        if(moved > (max_players / 2))
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][LadderTest] the duration bound moved the tier for MOST player counts - it, not the player count or the loot density, is choosing the opening circle. Set match_seconds_per_player / match_min_seconds / match_max_seconds against the range reported above.");

        WarnIfDurationWindowUnreachable(shortest, longest);

        //--- Put the memo back. The walk left it holding max_players, and the match about to be played
        //--- must recompute rather than inherit a self test's last answer.
        BattleRoyaleState.ResetDynamicZoneMemo();
    }

    //--- Split out of RunLadderSelfTest purely for scope: EnfusionScript allows one declaration per
    //--- name per method scope, and that method has already spent the obvious ones.
    protected void WarnIfDurationWindowUnreachable(int shortest, int longest)
    {
        BattleRoyaleZoneData settings;

        if(!m_ZoneSettings || !m_ZoneSettings.bound_match_duration)
            return;

        settings = m_ZoneSettings;

        if(longest > 0 && settings.match_max_seconds < longest)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][LadderTest] match_max_seconds (" + settings.match_max_seconds + ") is below the longest match this ladder can play (" + longest + " s), so the LARGEST circle can never be chosen at any population. Raise it, or shorten static_timers.");

        if(shortest > 0 && settings.match_min_seconds > shortest)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][LadderTest] match_min_seconds (" + settings.match_min_seconds + ") is above the shortest match this ladder can play (" + shortest + " s), so the lengthening walk can never be satisfied. Lower it, or raise min_zone_num.");
    }

    //--- Build every circle. Runs once per process; every later call is a lookup.
    protected void GenerateAll()
    {
        array<vector> chain = new array<vector>();

        ResetGenerationStats();
        BuildFeasiblePOIList();

        if(!BuildChain(chain))
        {
            //--- Only reachable with an empty static_sizes, which Validate() already clamps against.
            BattleRoyaleUtils.Warn("[BattleRoyaleZone] could not build a zone chain at all - zones will be placeholders. Check zone_settings.static_sizes.");
            return;
        }

        CommitChain(chain);

        //--- Below CommitChain, because both measure the COMMITTED circles, and below
        //--- BuildFeasiblePOIList above, because BuildDerivedLadder reads s_POI for its fallback.
        //--- The census must precede the ladder: it is the ladder's density input.
        CensusBuildings();
        BuildDerivedLadder();

        LogGeneratedChain();
        LogDerivedLadder();
    }

    //--- Seed handling + an explicit, logged boot step.
    //---
    //--- Called from BattleRoyaleServer.Init() so generation stops being an invisible side effect of
    //--- constructing round 0, and so the summary lands in the .rpt in a readable order. Calling it
    //--- is optional: GetBattleRoyalePlayAreas still generates on demand if nothing called this.
    static int s_GenerationSeed;
    static bool s_Prepared;

    static void PrepareGeneration()
    {
        int configured;
        int runs;
        int ladder_players;
        BattleRoyaleConfig config = BattleRoyaleConfig.GetConfig();

        if(s_Prepared)
            return;

        s_Prepared = true;

        if(!config || !config.GetZoneData())
            return;

        configured = config.GetZoneData().zone_generation_seed;

        if(configured != 0)
        {
            //--- Replaying a specific layout. NOTE Math.Randomize reseeds the GLOBAL generator, so
            //--- this also fixes loot, weather and spawn placement - which is why it is not the
            //--- default. It is a debugging tool for reproducing one bad layout.
            Math.Randomize(configured);
            s_GenerationSeed = configured;
            BattleRoyaleUtils.Info("[BattleRoyaleZone] zone_generation_seed = " + configured + " - replaying a fixed layout. This also fixes loot, weather and spawn placement; set it back to 0 for normal play.");
        }
        else
        {
            //--- Draw a seed from the engine's already-randomly-seeded generator and apply it. Every
            //--- boot still gets a different layout, so nothing observable changes - but the run
            //--- becomes REPLAYABLE, which it was not before, and that is the whole point: a layout
            //--- that dead-ends could never be reproduced, which is most of why this was hard to fix.
            //--- Deliberately NOT Math.Randomize(-1): the proto doc says it returns a new seed but
            //--- what it does with a negative argument is unverified, and a value we chose ourselves
            //--- is a value we know how to feed back in.
            s_GenerationSeed = Math.RandomInt(1, 2000000000);
            Math.Randomize(s_GenerationSeed);
            BattleRoyaleUtils.Info("[BattleRoyaleZone] generation seed " + s_GenerationSeed + " - put this in zone_settings.zone_generation_seed to replay this exact layout.");
        }

        //--- Force the circles to be built here rather than lazily inside round 0's constructor.
        BattleRoyaleZone zone = BattleRoyaleZone.GetZone(1);

        //--- Opt-in diagnostic, after the real chain is committed so it cannot disturb it.
        runs = config.GetZoneData().zone_selftest_runs;
        if(zone && runs > 0)
            zone.RunSelfTest(runs);

        //--- The ladder gate (#284). AFTER RunSelfTest, because that one churns s_ChainRadii 200 times
        //--- and restores it at the end; this one reads the committed circles and would otherwise be
        //--- reporting radii from a throwaway chain.
        ladder_players = config.GetZoneData().zone_ladder_selftest_players;
        if(zone && ladder_players > 0)
            zone.RunLadderSelfTest(ladder_players);
    }

	BattleRoyalePlayArea GetBattleRoyalePlayAreas(int zone_number)
	{
		if(!m_PlayAreas)
			GenerateAll();

		if(!m_PlayAreas)
			return NULL;

		if(zone_number < 0 || zone_number >= m_PlayAreas.Count())
		{
			BattleRoyaleUtils.Warn("Asked for play area " + zone_number + " but only " + m_PlayAreas.Count() + " were generated!");
			return NULL;
		}

		return m_PlayAreas[zone_number];
	}

    static ref set<ref array<float>> s_POI;

	// Initialize the POIs if they haven't been loaded already
	void InitializePOIs()
	{
		if(s_POI)
			return; // POIs already initialized

		s_POI = new set<ref array<float>>;
		string cfg = "CfgWorlds " + GetGame().GetWorldName() + " Names";

		BattleRoyaleUtils.Trace("Initializing POIs");
		BattleRoyaleUtils.Trace("Avoid Type Count: " + a_avoidType.Count());
		BattleRoyaleUtils.Trace("Avoid City Count: " + a_avoidCity.Count());

		//--- Hoisted: this was re-evaluated as the loop condition on every iteration, and it is a
		//--- native config walk.
		s_POIRawCount = GetGame().ConfigGetChildrenCount(cfg);

		//--- Arguments are evaluated BEFORE Trace() gets to check the log level, so the two
		//--- ConfigGetTextOut + string.Format pairs below were paid on every boot at every log level,
		//--- purely to build strings that were then thrown away. Resolved once here instead.
		bool trace_pois = BattleRoyaleUtils.CheckLogLevel(BattleRoyaleUtils.TRACE);

		BattleRoyaleUtils.Trace(string.Format("Loading %1 POIs", s_POIRawCount));
		for (int i = 0; i < s_POIRawCount; i++)
		{
			string city;
			GetGame().ConfigGetChildName(cfg, i, city);

			TFloatArray city_position = {};
			GetGame().ConfigGetFloatArray(string.Format("%1 %2 position", cfg, city), city_position);
			string poi_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city));

			if(a_avoidType.Find(poi_type) != -1 || a_avoidCity.Find(city) != -1)
			{
				if(trace_pois)
					BattleRoyaleUtils.Trace("Avoiding "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+city_position+" "+poi_type);
				continue;
			}

			vector override_position = m_Config.GetPOIsData().GetOverrodePosition( city );
			if( override_position != "0 0 0" )
			{
				//--- s_POI holds CfgWorlds' 2-element [x, z] pairs, read back as poi[0]/poi[1].
				//--- Writing a 3-element vector here put every overridden POI at z = 0 (the sea).
				city_position = {override_position[0], override_position[2]};
				BattleRoyaleUtils.Trace("Override " + city + " position!");
			}
			else if( city_position.Count() >= 2 )
			{
				//--- Move the POI onto the town's real centre, when the resolver found one.
				//---
				//--- end_in_villages seeds the final circle on this point with only
				//--- BR_ZONE_POI_JITTER_M of jitter, and a CfgWorlds position is a map-LABEL anchor
				//--- placed to keep the text off the buildings - so without this the endgame is
				//--- centred on the field beside the town it names. Measured on ChernarusPlus:
				//--- Chernogorsk's label has two buildings within 100 m of it.
				//---
				//--- Deliberately BELOW the admin override, and deliberately not folded into it: the
				//--- resolver's own table already contains admin overrides, but it is empty when
				//--- resolve_poi_from_buildings is off, and reading it alone would then silently
				//--- disable override_poi_positions along with the feature.
				//---
				//--- Same 2-element rule as above - GetAnchor returns a full vector and only x and z
				//--- may be written back.
				vector poi_label = "0 0 0";
				poi_label[0] = city_position[0];
				poi_label[2] = city_position[1];

				vector poi_anchor = BattleRoyalePOIResolver.GetAnchor( city, poi_label );
				if( poi_anchor != poi_label )
				{
					city_position = {poi_anchor[0], poi_anchor[2]};
					if(trace_pois)
						BattleRoyaleUtils.Trace("Resolved " + city + " onto its buildings at " + city_position);
				}
			}

			if(trace_pois)
				BattleRoyaleUtils.Trace("cfg "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+city_position+" "+poi_type);

			s_POI.Insert(city_position);
		}

		BattleRoyaleUtils.Trace("Loaded " + s_POI.Count() + " POIs");
	}

	bool IsPointInPolygon(vector point, array<vector> polygon)
	{
		if (!polygon || polygon.Count() < 3)
			return false;

		int i, j;
		bool result = false;
		j = polygon.Count() - 1;
		for (i = 0; i < polygon.Count(); i++)
		{
			vector vtx_i = polygon[i];
			vector vtx_j = polygon[j];
			// Only compare x and z coordinates (ignore y/height)
			bool crossesZLine = (vtx_i[2] > point[2]) != (vtx_j[2] > point[2]);

			if (crossesZLine)
			{
				float intersectX = vtx_i[0];
				float zDiff = vtx_j[2] - vtx_i[2];

				if (zDiff != 0)
				{
					float xDiff = vtx_j[0] - vtx_i[0];
					float ratio = (point[2] - vtx_i[2]) / zDiff;
					intersectX = vtx_i[0] + (xDiff * ratio);
				}

				if (point[0] < intersectX)
				{
					result = !result;
				}
			}
			j = i;
		}

		return result;
	}

	bool IsValidFinalZonePosition(vector position)
	{
		// If restriction is not enabled or no polygon is defined, any position is valid
		if (!m_ZoneSettings.restrict_final_zone || !m_ZoneSettings.final_zone_polygon || m_ZoneSettings.final_zone_polygon.Count() < 3)
			return true;

		// Check if the position is within the defined polygon
		return IsPointInPolygon(position, polygon_vertices);
	}
}
