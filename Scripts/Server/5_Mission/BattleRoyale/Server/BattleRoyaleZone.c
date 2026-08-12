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

        m_PlayArea = new BattleRoyalePlayArea(Vector(0,0,0), 0.0);

        //--- Cache the world geometry before anything that searches in it.
        f_WorldSize = GetGame().GetWorld().GetWorldSize();
        m_MapCenter = "0 0 0";
        m_MapCenter[0] = f_WorldSize / 2;
        m_MapCenter[2] = f_WorldSize / 2;
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

    static ref BattleRoyaleZone GetZone(int x = 1)
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
                m_Zone = new BattleRoyaleZone(m_Zones.Get(z_Index - 1));
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

    ref BattleRoyalePlayArea GetArea()
    {
        return m_PlayArea;
    }

    ref BattleRoyaleZone GetParent()
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
        ref BattleRoyaleZone parent = m_ParentZone;
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

    int GetZoneTimer()
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

            return a_StaticTimers[timer_index] + GetDurationOffset(timer_index);
        }

        return 60 * i_RoundDurationMinutes;
    }

    int GetZoneMinPlayers()
    {
        int min_players_index = GetZoneSettingsIndex();
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
    //=====================================================================================

    //--- How far circle `level` may move from circle `level-1`. Containment alone would permit the
    //--- full (r_i - r_{i-1}); BR_ZONE_REACH_PERCENT is how much of that the chain plans on.
    protected float GetStepReach(int level)
    {
        if(level < 1 || level >= i_NumRounds)
            return 0;

        if(!a_StaticSizes || level >= a_StaticSizes.Count())
            return 0;

        return BR_ZONE_REACH_PERCENT * (a_StaticSizes[level] - a_StaticSizes[level - 1]);
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

        if(!a_StaticSizes)
            return false;

        for(i = level + 1; i < i_NumRounds; i++)
        {
            if(i >= a_StaticSizes.Count())
                return false;

            p = StepToward(p, m_MapCenter, GetStepReach(i));

            if(!FitsWorld(p, a_StaticSizes[i]))
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

        if(reach <= 0)
            return 0;

        if(!a_StaticSizes || level >= a_StaticSizes.Count())
            return 0;

        //--- No movement needed at all is the common case; answer it without bisecting.
        probe = StepToward(parent_center, m_MapCenter, 0);
        if(FitsWorld(probe, a_StaticSizes[level]) && CanChainComplete(probe, level))
            return 0;

        hi = reach;
        for(i = 0; i < 5; i++)
        {
            mid = (lo + hi) * 0.5;
            probe = StepToward(parent_center, m_MapCenter, mid);

            if(FitsWorld(probe, a_StaticSizes[level]) && CanChainComplete(probe, level))
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
            ref array<float> poi = s_POI.Get(i);
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

    //--- Parallel to m_PlayAreas: extra round seconds earned by the travel into each circle.
    //--- Static for the same reason m_PlayAreas is - the circles are generated once per process and
    //--- every zone object reads the same set.
    static ref array<float> s_PlayAreaDurationOffsets;

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
    static ref array<int> s_TierUsage;   //--- [0]=T1 [1]=T2 [2]=T3 [3]=sweep [4]=witness step
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
        for(i = 0; i < 5; i++)
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
        float radius = a_StaticSizes[level];
        float span = radius - a_StaticSizes[level - 1];
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

    //--- Place circle `level` around its parent. `desperation` is how many times this level has
    //--- already been attempted against its CURRENT parent; tiers escalate inside one call, and the
    //--- witness step only unlocks once backtracking has had its chance, so a healthy match never
    //--- reaches it.
    protected bool TryPlaceLevel(int level, vector parent_center, int desperation, out vector placed)
    {
        float radius = a_StaticSizes[level];
        float span = radius - a_StaticSizes[level - 1];
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
        }

        if(SweepPlaceLevel(level, parent_center, centre_dir, placed))
        {
            s_TierUsage.Set(3, s_TierUsage.Get(3) + 1);
            return true;
        }

        //--- Let the caller re-roll the parent first: a different parent is a different annulus, and
        //--- that is far more likely to help than another pass over this one.
        if(desperation < BR_ZONE_LEVEL_RETRIES)
            return false;

        placed = WitnessStep(level, parent_center);
        placed[1] = GetGame().SurfaceY(placed[0], placed[2]);
        s_TierUsage.Set(4, s_TierUsage.Get(4) + 1);

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
                ref array<float> poi = s_FeasiblePOI.Get(idx);
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
            }

            if(level >= n)
                return true;
        }

        return false;
    }

    //--- Turn a finished chain into the play areas, and work out the round-time bonuses from it.
    //---
    //--- Offsets are computed HERE rather than during placement because a single scratch slot cannot
    //--- survive backtracking - a re-rolled level's offset has to be discarded, and the old code had
    //--- no way to do that.
    protected void CommitChain(notnull array<vector> chain)
    {
        int i;
        float dist;
        float offset;
        float radius;
        vector centre;
        vector from_2d;
        vector to_2d;

        m_PlayAreas = new array<ref BattleRoyalePlayArea>();
        s_PlayAreaDurationOffsets = new array<float>();

        for(i = 0; i < chain.Count(); i++)
        {
            //--- Both reads into locals before the constructor call, as everywhere else here.
            centre = chain.Get(i);
            radius = a_StaticSizes[i];

            m_PlayAreas.Insert(new BattleRoyalePlayArea(centre, radius));
            s_PlayAreaDurationOffsets.Insert(0);
        }

        //--- The travel between circle i and circle i-1 belongs to the round that moves players INTO
        //--- circle i-1, i.e. play area index i-1. Measured in 2D: the old code compared positions
        //--- whose Y came from SurfaceY, so hilly terrain inflated the distance.
        for(i = 1; i < chain.Count(); i++)
        {
            from_2d = chain.Get(i);
            to_2d = chain.Get(i - 1);
            from_2d[1] = 0;
            to_2d[1] = 0;

            dist = vector.Distance(from_2d, to_2d);
            if(dist <= BR_ZONE_OFFSET_MIN_DISTANCE)
                continue;

            offset = dist / BR_ZONE_OFFSET_SPEED_MPS;
            if(offset > BR_ZONE_OFFSET_MAX_SECONDS)
                offset = BR_ZONE_OFFSET_MAX_SECONDS;

            s_PlayAreaDurationOffsets.Set(i - 1, offset);
        }
    }

    protected void LogGeneratedChain()
    {
        int i;
        int surface_calls = s_SurfaceCalls;

        BattleRoyaleUtils.Info("[BattleRoyaleZone] generated " + m_PlayAreas.Count() + " circles: " + s_TotalWork + " placements, " + s_Backtracks + " backtracks (deepest level " + s_DeepestBacktrack + "), " + s_SeedsUsed + " seed(s), " + surface_calls + " surface queries.");
        BattleRoyaleUtils.Info("[BattleRoyaleZone] tier usage: T1 " + s_TierUsage.Get(0) + "  T2 " + s_TierUsage.Get(1) + "  T3 " + s_TierUsage.Get(2) + "  sweep " + s_TierUsage.Get(3) + "  fallback-step " + s_TierUsage.Get(4) + ".");

        //--- Index 0 is the FINAL circle - the array is smallest-first, like the settings arrays.
        for(i = 0; i < m_PlayAreas.Count(); i++)
        {
            BattleRoyaleUtils.Debug("[BattleRoyaleZone]   [" + i + "] center=" + m_PlayAreas.Get(i).GetCenter() + " radius=" + m_PlayAreas.Get(i).GetRadius() + " duration_offset=" + s_PlayAreaDurationOffsets.Get(i));
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
        ref array<vector> chain = new array<vector>();
        ref array<int> depth_hist = new array<int>();
        ref array<int> seed_hist = new array<int>();
        ref array<int> tier_total = new array<int>();
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

        if(iterations < 1)
            return;

        for(k = 0; k <= i_NumRounds; k++)
        {
            depth_hist.Insert(0);
        }
        for(k = 0; k <= BR_ZONE_MAX_SEEDS; k++)
        {
            seed_hist.Insert(0);
        }
        for(k = 0; k < 5; k++)
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

            if(depth >= 0 && depth < depth_hist.Count())
                depth_hist.Set(depth, depth_hist.Get(depth) + 1);
            if(seeds >= 0 && seeds < seed_hist.Count())
                seed_hist.Set(seeds, seed_hist.Get(seeds) + 1);

            for(k = 0; k < 5; k++)
            {
                tier_total.Set(k, tier_total.Get(k) + s_TierUsage.Get(k));
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

        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest]   tier per circle: T1 " + tier_total.Get(0) + "  T2 " + tier_total.Get(1) + "  T3 " + tier_total.Get(2) + "  sweep " + tier_total.Get(3) + "  fallback-step " + tier_total.Get(4) + ".");
        BattleRoyaleUtils.Info("[BattleRoyaleZone][SelfTest]   final circle spread: x[" + seed_min_x + " .. " + seed_max_x + "] z[" + seed_min_z + " .. " + seed_max_z + "].");

        if((seed_max_x - seed_min_x) < 1.0 && (seed_max_z - seed_min_z) < 1.0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][SelfTest] every run produced the SAME final circle - generation is not varying between matches. Expect the same endgame location every time.");

        if(capped > 0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][SelfTest] stopped early at the work cap (" + BR_ZONE_SELFTEST_WORK_CAP + " placements) - this map is costing far more search than a healthy one.");

        if(failed > 0)
            BattleRoyaleUtils.Warn("[BattleRoyaleZone][SelfTest] " + failed + " run(s) produced NO chain at all. That should be impossible - check static_sizes against this map's size.");

        //--- Leave the RNG somewhere fresh so the match about to be played is not correlated with the
        //--- stream this test just consumed.
        Math.Randomize(Math.RandomInt(1, 2000000000));
    }

    //--- Build every circle. Runs once per process; every later call is a lookup.
    protected void GenerateAll()
    {
        ref array<vector> chain = new array<vector>();

        ResetGenerationStats();
        BuildFeasiblePOIList();

        if(!BuildChain(chain))
        {
            //--- Only reachable with an empty static_sizes, which Validate() already clamps against.
            BattleRoyaleUtils.Warn("[BattleRoyaleZone] could not build a zone chain at all - zones will be placeholders. Check zone_settings.static_sizes.");
            return;
        }

        CommitChain(chain);
        LogGeneratedChain();
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
