#ifdef SERVER

//=========================================================================================
//--- CE lifetime probe. Answers ONE question: does GetCEApi().TimeShift() actually do
//--- anything on a live dedicated server?
//---
//--- WHY THIS EXISTS
//--- TimeShift is the only script-reachable way to expire every item on the map at once, and
//--- expiry is what makes CE re-roll the loot without restarting the process. But TimeShift,
//--- CleanMap and LootDepleteLifetime have ZERO call sites anywhere in P:\scripts - the exact
//--- shape that burned us with OverrideAimChangeX/Y, which compiles, runs, and silently does
//--- nothing. Nothing about reading the header tells us which of those two this is.
//---
//--- WHAT MAKES THIS A PROBE AND NOT A GUESS
//--- A bare before/after on TimeShift cannot distinguish "TimeShift is a no-op" from "the
//--- instrument is blind" from "CE cleanup is not running at all right now". So the probe runs
//--- a POSITIVE CONTROL first: RadiusLifetimeDecrease on a small radius around sample point #0
//--- only. That call is proven live by vanilla itself - the flag refresher calls its sibling
//--- RadiusLifetimeReset on a real server every FlagRefreshFrequency (totem.c:168). So:
//---
//---   point #0 drains, points #1..n do not  ->  instrument works AND the family is live
//---   nothing drains anywhere               ->  instrument blind, or cleanup is not running
//---   control drains, TimeShift does not    ->  TimeShift specifically is the no-op
//---
//--- That third line is the actionable outcome: it means tile RadiusLifetimeDecrease over the
//--- map instead of looking for a global call.
//---
//--- IT ALSO MEASURES THE DRAIN RATE, WHICH IS THE NUMBER THAT DECIDES THE FEATURE
//--- After TimeShift the probe keeps sampling and reports items/s plus how long the sampled
//--- population would take to empty. That comparison against a restart is the actual decision,
//--- independent of whether TimeShift works at all.
//---
//--- ============ WHAT THE FIRST REAL RUN FOUND (2026-08-15, ChernarusPlus) ============
//--- BOTH CALLS FIRE. The control was textbook: point #0 went 121 -> 90 monotonically over 90 s
//--- while all seven untargeted points sat perfectly still. Then TimeShift moved all eight.
//---
//--- AND THE FEATURE IS STILL DEAD, for a reason no API change can fix: CE cleanup is
//--- rate-limited. The drain ran -90 items in 241 s across points holding 1503 of the map's
//--- 33,959 items - 0.373 items/s sampled, ~8.4 items/s map-wide, i.e. roughly 67 MINUTES to
//--- expire the map, before the throttled refill even begins. A restart is ~76 s.
//---
//--- So the probe's remaining value is as a measuring instrument, not as a gate on a feature.
//--- Note the baseline was frozen at 1503 across four consecutive samples, which is what makes
//--- the post-TimeShift decline attributable rather than churn - keep the settle phase.
//--- ====================================================================================
//---
//--- HOW TO RUN IT
//---   LaunchLocalMP.bat 1, with -br-ceprobe -br-info on the SERVER command line.
//---
//--- ONE CLIENT MUST BE CONNECTED. globals.xml has IdleModeStartup=1 / IdleModeCountdown=60,
//--- so an empty server drops into idle mode and CE stops - which would look exactly like
//--- "TimeShift does nothing". The probe logs a player count per sample point so an idle or
//--- avoidance-shadowed run is visible rather than silently wrong. It also does NOT need the
//--- match to start: one connected player sits in 1_BattleRoyaleDebug forever, which is a
//--- stable environment to measure in.
//---
//--- Sample points come from CfgWorlds Names, so this works on Sakhal unchanged.
//---
//--- Constants live here rather than in BattleRoyaleConstants.c on purpose: this is a
//--- throwaway diagnostic, and none of these numbers means anything outside it.
//=========================================================================================

//! Wall-clock spacing between samples, ms.
const int BR_CEPROBE_SAMPLE_MS = 10000;
//! Hard cap on the settle wait before the probe acts anyway, ms. Acting during CE's initial
//! spawn would confound everything, so the probe waits for the count to stop climbing first.
const int BR_CEPROBE_SETTLE_MAX_MS = 180000;
//! How long to watch after the scoped control call, ms.
const int BR_CEPROBE_CONTROL_MS = 90000;
//! How long to watch after TimeShift, ms. Longer than the control window because this one is
//! also measuring the refill, not just whether the drain happened.
const int BR_CEPROBE_TIMESHIFT_MS = 240000;
//! Sphere radius counted around each sample point, m.
const float BR_CEPROBE_SAMPLE_RADIUS_M = 150;
//! Radius handed to RadiusLifetimeDecrease for the scoped control, m. Comfortably larger than
//! the sample radius so the control cannot half-cover the thing it is being measured by.
const float BR_CEPROBE_CONTROL_RADIUS_M = 250;
//! Seconds subtracted from lifetimes. Above the longest lifetime in the mission's types.xml
//! (28800), so every item in range goes straight to zero.
const float BR_CEPROBE_LIFETIME_SUBTRACT = 999999;
//! Sample points wanted, and the minimum separation between them.
const int BR_CEPROBE_POINTS = 8;
const float BR_CEPROBE_POINT_SEPARATION_M = 800;
//! Consecutive samples within tolerance before CE counts as settled.
const int BR_CEPROBE_SETTLE_SAMPLES = 3;
const float BR_CEPROBE_SETTLE_TOLERANCE = 0.02;
//! What counts as "this point fell". Deliberately a NOISE FLOOR and not a drain fraction.
//!
//! This was a fixed 0.5 - a point had to lose half its items - and it produced a WRONG VERDICT on
//! the first real run (2026-08-15): the control point fell 121 -> 90 monotonically while all seven
//! others sat still, which is a textbook positive control, and the probe reported CONTROL FAILED
//! because 26% < 50%. CE cleanup is rate-limited, so the drain does not complete inside any window
//! worth waiting for and NO fixed fraction is safe.
//!
//! The differential is the real evidence and it is scale-free: did the targeted point fall while
//! the untargeted ones held. So these two only have to clear the sampling noise, which was
//! observed at +/-2 items per point.
const int BR_CEPROBE_FELL_MIN_ITEMS = 3;
const float BR_CEPROBE_FELL_MIN_FRAC = 0.02;

const int BR_CEPROBE_PHASE_IDLE = 0;
const int BR_CEPROBE_PHASE_SETTLE = 1;
const int BR_CEPROBE_PHASE_CONTROL = 2;
const int BR_CEPROBE_PHASE_TIMESHIFT = 3;
const int BR_CEPROBE_PHASE_DONE = 4;

class BattleRoyaleCEProbe
{
    protected static bool s_Checked;
    protected static bool s_Enabled;
    protected static int s_Phase;
    protected static int s_NextSampleMs;
    protected static int s_PhaseStartMs;
    protected static int s_SettleRun;
    protected static int s_LastTotal;
    protected static bool s_Settled;
    protected static int s_TroughTotal;
    protected static int s_TroughMs;

    protected static ref array<vector> s_Points;
    //! Counts at the moment the control fired.
    protected static ref array<int> s_Baseline;
    //! Counts at the moment TimeShift fired.
    protected static ref array<int> s_AfterControl;
    //! Counts from the most recent sample.
    protected static ref array<int> s_Latest;

    //=====================================================================================
    //--- Driven from the existing 10 Hz block in BattleRoyaleServer.Update(). Costs one bool
    //--- test per tick when the CLI flag is absent, which is every real server.
    //=====================================================================================
    static void Tick()
    {
        int now;

        if (!s_Checked)
        {
            s_Checked = true;
            s_Enabled = IsCLIParam("br-ceprobe");

            if (s_Enabled)
                Begin();
        }

        if (!s_Enabled)
            return;

        if (s_Phase == BR_CEPROBE_PHASE_DONE)
            return;

        now = GetGame().GetTime();

        if (now < s_NextSampleMs)
            return;

        s_NextSampleMs = now + BR_CEPROBE_SAMPLE_MS;
        Sample(now);
    }

    protected static void Begin()
    {
        string header;

        s_Phase = BR_CEPROBE_PHASE_SETTLE;
        s_PhaseStartMs = GetGame().GetTime();
        s_LastTotal = -1;
        s_TroughTotal = -1;

        PickPoints();

        header = "[CEProbe] armed on " + GetGame().GetWorldName();
        header = header + " - " + s_Points.Count() + " sample points, r=" + BR_CEPROBE_SAMPLE_RADIUS_M + " m.";
        BattleRoyaleUtils.Info(header);

        if (s_Points.Count() < 2)
        {
            BattleRoyaleUtils.Warn("[CEProbe] ABORT: fewer than 2 sample points resolved from CfgWorlds Names. The control needs point #0 to drain while the others do not, so one point cannot answer anything.");
            s_Phase = BR_CEPROBE_PHASE_DONE;
            return;
        }

        BattleRoyaleUtils.Info("[CEProbe] waiting for CE to settle before touching anything. Point #0 is the control point.");
    }

    //=====================================================================================
    //--- Sample points from CfgWorlds Names, settlements first. Two passes rather than one
    //--- filtered pass so a map whose Names are mostly terrain features still yields points
    //--- instead of none.
    //=====================================================================================
    protected static void PickPoints()
    {
        string cfg;
        string child;
        string poi_type;
        int count;
        int pass;
        int i;
        int j;
        bool is_settlement;
        bool too_close;

        s_Points = new array<vector>();

        cfg = "CfgWorlds " + GetGame().GetWorldName() + " Names";
        count = GetGame().ConfigGetChildrenCount(cfg);

        for (pass = 0; pass < 2; pass++)
        {
            for (i = 0; i < count; i++)
            {
                if (s_Points.Count() >= BR_CEPROBE_POINTS)
                    break;

                GetGame().ConfigGetChildName(cfg, i, child);
                poi_type = GetGame().ConfigGetTextOut(cfg + " " + child + " type");

                is_settlement = false;
                if (poi_type == "City")
                    is_settlement = true;
                if (poi_type == "Village")
                    is_settlement = true;
                if (poi_type == "Capital")
                    is_settlement = true;
                if (poi_type == "Local")
                    is_settlement = true;

                if (pass == 0 && !is_settlement)
                    continue;
                if (pass == 1 && is_settlement)
                    continue;

                TFloatArray raw = {};
                GetGame().ConfigGetFloatArray(cfg + " " + child + " position", raw);

                if (raw.Count() < 2)
                    continue;

                //--- CfgWorlds positions are 2-element [x, z] pairs. Writing a 3-element vector
                //--- straight from them puts every point at z = 0, i.e. in the sea - the same
                //--- trap BattleRoyaleZone.InitializePOIs carries a comment about.
                vector point = "0 0 0";
                point[0] = raw.Get(0);
                point[2] = raw.Get(1);
                point[1] = GetGame().SurfaceY(point[0], point[2]);

                too_close = false;
                for (j = 0; j < s_Points.Count(); j++)
                {
                    if (vector.Distance(s_Points.Get(j), point) < BR_CEPROBE_POINT_SEPARATION_M)
                        too_close = true;
                }

                if (too_close)
                    continue;

                s_Points.Insert(point);
            }
        }
    }

    //=====================================================================================
    //--- One sample, then whatever the current phase wants to do about it.
    //=====================================================================================
    protected static void Sample(int now)
    {
        int i;
        int total;
        int elapsed;
        int count;
        int players;
        float drift;
        float last;
        string line;
        string detail;

        if (!s_Latest)
            s_Latest = new array<int>();

        s_Latest.Clear();
        total = 0;
        detail = "[CEProbe]   ";

        for (i = 0; i < s_Points.Count(); i++)
        {
            count = CountAt(s_Points.Get(i));
            players = PlayersNear(s_Points.Get(i));

            s_Latest.Insert(count);
            total = total + count;

            detail = detail + "#" + i + "=" + count;

            //--- CleanupAvoidance and LootSpawnAvoidance are both 100 m in globals.xml, so a
            //--- point with a player on it neither drains nor refills. Flagged rather than
            //--- excluded: a run where that is happening should look wrong, not look clean.
            if (players > 0)
                detail = detail + "[P" + players + "]";

            detail = detail + "  ";
        }

        elapsed = (now - s_PhaseStartMs) / 1000;

        line = "[CEProbe] " + PhaseName() + " t+" + elapsed + "s";
        line = line + " total=" + total;
        BattleRoyaleUtils.Info(line);
        BattleRoyaleUtils.Info(detail);

        if (s_Phase == BR_CEPROBE_PHASE_SETTLE)
        {
            //--- Acting while CE is still doing its initial spawn would put a rising count under
            //--- the drain and make everything after it unreadable.
            drift = 1;

            if (s_LastTotal > 0)
            {
                last = s_LastTotal;
                drift = Math.AbsFloat(total - last) / last;
            }

            if (drift <= BR_CEPROBE_SETTLE_TOLERANCE)
                s_SettleRun++;
            else
                s_SettleRun = 0;

            s_LastTotal = total;

            if (s_SettleRun >= BR_CEPROBE_SETTLE_SAMPLES)
            {
                s_Settled = true;
                FireControl(now);
                return;
            }

            if ((now - s_PhaseStartMs) > BR_CEPROBE_SETTLE_MAX_MS)
            {
                s_Settled = false;
                BattleRoyaleUtils.Warn("[CEProbe] CE never settled within " + (BR_CEPROBE_SETTLE_MAX_MS / 1000) + "s - the count is still moving on its own. Everything below is measured against a moving baseline; treat a weak result as inconclusive rather than negative.");
                FireControl(now);
            }

            return;
        }

        if (s_Phase == BR_CEPROBE_PHASE_CONTROL)
        {
            if ((now - s_PhaseStartMs) > BR_CEPROBE_CONTROL_MS)
                FireTimeShift(now);

            return;
        }

        if (s_Phase == BR_CEPROBE_PHASE_TIMESHIFT)
        {
            if (s_TroughTotal < 0 || total < s_TroughTotal)
            {
                s_TroughTotal = total;
                s_TroughMs = now;
            }

            if ((now - s_PhaseStartMs) > BR_CEPROBE_TIMESHIFT_MS)
                Verdict();
        }
    }

    //=====================================================================================
    //--- Stage 1: the positive control. Scoped to point #0 only, which is what makes it a
    //--- control - the other points are the untouched comparison in the same run.
    //=====================================================================================
    protected static void FireControl(int now)
    {
        CEApi ce;

        ce = GetCEApi();

        if (!ce)
        {
            BattleRoyaleUtils.Warn("[CEProbe] ABORT: GetCEApi() is NULL. This mission has no Hive, so there is no economy here to probe - check MPMission points at a mission whose init.c calls CreateHive().");
            s_Phase = BR_CEPROBE_PHASE_DONE;
            return;
        }

        s_Baseline = Snapshot(s_Latest);

        BattleRoyaleUtils.Info("[CEProbe] --- STAGE 1: RadiusLifetimeDecrease at point #0, r=" + BR_CEPROBE_CONTROL_RADIUS_M + " m, -" + BR_CEPROBE_LIFETIME_SUBTRACT + "s. Expect #0 to fall and every other point to hold.");

        ce.RadiusLifetimeDecrease(s_Points.Get(0), BR_CEPROBE_CONTROL_RADIUS_M, BR_CEPROBE_LIFETIME_SUBTRACT);

        s_Phase = BR_CEPROBE_PHASE_CONTROL;
        s_PhaseStartMs = now;
    }

    //=====================================================================================
    //--- Stage 2: the thing actually under test.
    //=====================================================================================
    protected static void FireTimeShift(int now)
    {
        CEApi ce;

        ce = GetCEApi();

        if (!ce)
        {
            BattleRoyaleUtils.Warn("[CEProbe] ABORT: GetCEApi() went NULL between stages.");
            s_Phase = BR_CEPROBE_PHASE_DONE;
            return;
        }

        s_AfterControl = Snapshot(s_Latest);

        BattleRoyaleUtils.Info("[CEProbe] --- STAGE 2: TimeShift(-" + BR_CEPROBE_LIFETIME_SUBTRACT + "s) over the whole world. Expect EVERY point to fall.");

        ce.TimeShift(BR_CEPROBE_LIFETIME_SUBTRACT);

        s_Phase = BR_CEPROBE_PHASE_TIMESHIFT;
        s_PhaseStartMs = now;
        s_TroughTotal = -1;
    }

    //=====================================================================================
    //--- The verdict. Every branch names itself, so "nothing arrived" and "everything arrived
    //--- and was thrown away" cannot both present as no line at all.
    //=====================================================================================
    protected static void Verdict()
    {
        int i;
        int baseline_total;
        int after_control_total;
        int now_total;
        int others_fell;
        int shift_fell;
        float lost_f;
        float window_s;
        float rate;
        float mins_to_empty;
        bool control_worked;
        string line;

        s_Phase = BR_CEPROBE_PHASE_DONE;

        baseline_total = SumOf(s_Baseline);
        after_control_total = SumOf(s_AfterControl);
        now_total = SumOf(s_Latest);
        control_worked = Fell(0, s_Baseline, s_AfterControl);
        others_fell = 0;
        shift_fell = 0;

        for (i = 1; i < s_Points.Count(); i++)
        {
            if (Fell(i, s_Baseline, s_AfterControl))
                others_fell++;
        }

        for (i = 0; i < s_Points.Count(); i++)
        {
            if (Fell(i, s_AfterControl, s_Latest))
                shift_fell++;
        }

        //--- Built in steps rather than as one expression: a single concatenation carrying this
        //--- many fields is exactly the shape that trips "Formula too complex", which packs fine
        //--- and only fails when the game loads the module.
        BattleRoyaleUtils.Info("[CEProbe] ===================== RESULT =====================");
        BattleRoyaleUtils.Info("[CEProbe] settled before acting: " + YN(s_Settled));

        line = "[CEProbe] totals: baseline=" + baseline_total;
        line = line + "  after control=" + after_control_total;
        line = line + "  now=" + now_total;
        BattleRoyaleUtils.Info(line);

        line = "[CEProbe] control point #0 fell=" + YN(control_worked);
        line = line + "  untargeted points that also fell=" + others_fell;
        line = line + " of " + (s_Points.Count() - 1);
        BattleRoyaleUtils.Info(line);

        line = "[CEProbe] points that fell under TimeShift=" + shift_fell;
        line = line + " of " + s_Points.Count();
        BattleRoyaleUtils.Info(line);

        if (baseline_total <= 0)
        {
            BattleRoyaleUtils.Info("[CEProbe] VERDICT: INSTRUMENT BLIND. Zero items counted anywhere before we touched a thing, so nothing after that means anything either way. Check the mission actually spawns loot and that the sample points are on land.");
            return;
        }

        if (!control_worked)
        {
            BattleRoyaleUtils.Info("[CEProbe] VERDICT: CONTROL FAILED - do not read anything into the TimeShift result. RadiusLifetimeDecrease is proven live by vanilla's own flag refresher, so the likely cause is that CE cleanup is not running at all: check a player was connected for the whole run (IdleModeStartup=1 stops CE on an empty server) and check the [P..] flags above.");
            return;
        }

        if (others_fell > 0)
        {
            BattleRoyaleUtils.Info("[CEProbe] WARNING: " + others_fell + " untargeted point(s) also fell during the control window. Either the points are closer together than the control radius, or something else is expiring loot. The stage 2 result below is weaker than it looks.");
        }

        if (shift_fell <= others_fell)
        {
            BattleRoyaleUtils.Info("[CEProbe] VERDICT: TIMESHIFT IS A NO-OP. The control fell on the same run with the same instrument, so the mechanism and the measurement both work and TimeShift specifically does nothing. ACTION: tile RadiusLifetimeDecrease across the map instead - it is the same family and it demonstrably fires.");
            return;
        }

        BattleRoyaleUtils.Info("[CEProbe] VERDICT: TIMESHIFT WORKS. It expired loot map-wide.");

        //--- The rate, not the depth, is the answer. Measured 2026-08-15 the drain does not come
        //--- close to finishing inside any sane window - 6.1% in four minutes - so reporting "how
        //--- far it fell" says more about the window length than about CE. Extrapolating to an
        //--- empty sampled population is the figure that compares directly against a restart.
        lost_f = baseline_total - now_total;
        window_s = (GetGame().GetTime() - s_PhaseStartMs) / 1000.0;
        rate = 0;
        mins_to_empty = 0;

        if (window_s > 0)
            rate = lost_f / window_s;

        if (rate > 0)
            mins_to_empty = (now_total / rate) / 60.0;

        line = "[CEProbe] DRAIN RATE: -" + lost_f + " items over " + window_s + "s";
        line = line + " = " + rate + " items/s across " + s_Points.Count() + " points.";
        BattleRoyaleUtils.Info(line);

        line = "[CEProbe] At that rate the SAMPLED population empties in ~" + mins_to_empty + " min";
        line = line + " - and the refill has not even started yet.";
        BattleRoyaleUtils.Info(line);
        BattleRoyaleUtils.Info("[CEProbe] Compare against a server restart (~76 s warm) before concluding this is worth doing. Scale to the whole map by dividing the sampled item count by the CE '[LootRespawner] Total in Map' figure from boot.");
    }

    //=====================================================================================
    //--- Helpers.
    //=====================================================================================

    //! Free-standing world loot in a sphere. An item in a player's hands or in another item's
    //! cargo has a hierarchy parent and is not what CE respawns onto the map.
    protected static int CountAt(vector center)
    {
        array<Object> objects;
        array<CargoBase> cargos;
        int found;
        int i;

        objects = new array<Object>();
        cargos = new array<CargoBase>();
        found = 0;

        GetGame().GetObjectsAtPosition3D(center, BR_CEPROBE_SAMPLE_RADIUS_M, objects, cargos);

        for (i = 0; i < objects.Count(); i++)
        {
            ItemBase item = ItemBase.Cast(objects.Get(i));

            if (!item)
                continue;

            if (item.GetHierarchyParent())
                continue;

            found++;
        }

        return found;
    }

    protected static int PlayersNear(vector center)
    {
        CEApi ce;

        ce = GetCEApi();

        if (!ce)
            return -1;

        return ce.CountPlayersWithinRange(center, BR_CEPROBE_SAMPLE_RADIUS_M);
    }

    //! Frozen copy of a sample. s_Latest is cleared and refilled every 10 s, so a stage that
    //! wants to compare against an earlier moment has to own its own array, not alias that one.
    protected static array<int> Snapshot(array<int> from)
    {
        array<int> copy;
        int i;

        copy = new array<int>();

        if (!from)
            return copy;

        for (i = 0; i < from.Count(); i++)
        {
            copy.Insert(from.Get(i));
        }

        return copy;
    }

    protected static int SumOf(array<int> counts)
    {
        int i;
        int total;

        total = 0;

        if (!counts)
            return 0;

        for (i = 0; i < counts.Count(); i++)
        {
            total = total + counts.Get(i);
        }

        return total;
    }

    //! Did point `index` fall by more than sampling noise between the two snapshots? Per-point
    //! rather than on the total, so one busy point cannot mask the rest, and a floor rather than a
    //! fraction-of-baseline - see BR_CEPROBE_FELL_MIN_ITEMS for why that distinction cost a run.
    protected static bool Fell(int index, array<int> before, array<int> after)
    {
        int lost;
        int floor_items;
        float floor_frac;

        if (!before || !after)
            return false;

        if (index >= before.Count() || index >= after.Count())
            return false;

        if (before.Get(index) <= 0)
            return false;

        lost = before.Get(index) - after.Get(index);

        if (lost < BR_CEPROBE_FELL_MIN_ITEMS)
            return false;

        //--- Scale the floor on a busy point, so a 400-item town needs more than 3 to count.
        floor_frac = before.Get(index) * BR_CEPROBE_FELL_MIN_FRAC;
        floor_items = BR_CEPROBE_FELL_MIN_ITEMS;

        if (floor_frac > floor_items)
            floor_items = floor_frac;

        return lost >= floor_items;
    }

    //! EnfusionScript has no bool -> string promotion, so a bare `"x=" + some_bool` is a compile
    //! error rather than the "x=1" it looks like.
    protected static string YN(bool value)
    {
        if (value)
            return "yes";

        return "no";
    }

    protected static string PhaseName()
    {
        if (s_Phase == BR_CEPROBE_PHASE_SETTLE)
            return "settle ";
        if (s_Phase == BR_CEPROBE_PHASE_CONTROL)
            return "control";
        if (s_Phase == BR_CEPROBE_PHASE_TIMESHIFT)
            return "shift  ";

        return "idle   ";
    }
}

#endif
