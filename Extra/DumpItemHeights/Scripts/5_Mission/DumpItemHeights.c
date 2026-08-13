#ifndef SERVER
#ifdef DIAG_DEVELOPER

/**
 *  DumpItemHeights - walks CfgVehicles, CfgWeapons and CfgMagazines, spawns every loot item once,
 *  measures its bounding box and writes the result to a CSV in the profile directory.
 *
 *  size_y is the column that matters: it is the model's vertical extent, i.e. what has to be
 *  compared against <point height="..."/> in mapgroupproto.xml.
 *
 *  THREE TREES, because loot is not all in one. Firearms live in CfgWeapons and magazines and loose
 *  rounds in CfgMagazines; only items and clothing are in CfgVehicles. All three spawn at loot
 *  points, so all three need heights.
 *
 *  THREE GATES, all of which must pass before a single object is created:
 *    1. #ifndef SERVER        - line 1. Client only.
 *    2. #ifdef DIAG_DEVELOPER - line 2. DayZDiag_x64 only.
 *    3. !GetGame().IsMultiplayer() - runtime, in Arm(). Offline only, i.e. LaunchOffline.bat.
 *
 *  CHUNKED, NOT SYNCHRONOUS. There are several thousand classes across the three trees; creating and
 *  deleting all the loot among them inside one frame is a multi-minute freeze the engine may well
 *  decide is a hang. Update() processes ITEMS_PER_FRAME config indices per frame instead.
 *
 *  RESUMABLE, BECAUSE CreateObjectEx CAN HARD-CRASH THE CLIENT AND THERE IS NO try/catch.
 *  Measured 2026-08-13: ItemOptics (scope=1, no model of its own - it inherits model="" from
 *  Inventory_Base) faults the engine with "Access violation. Illegal read ... at 0x31c" inside
 *  CreateObjectEx. The scope >= 2 filter below excludes that whole category, but nothing proves
 *  another of the several thousand classes is not also bad, so the position currently being created
 *  is written to a progress file before every attempt. A relaunch resumes at the next index, records
 *  the offender in the CSV as a comment and carries on - one relaunch per bad class rather than
 *  losing the run.
 */
class DumpItemHeights
{
    //--- Both land in the CLIENT profile directory (ClientProfileDirectory in Workbench/user.cfg).
    private static const string OUTPUT_PATH   = "$profile:item_heights.csv";
    private static const string PROGRESS_PATH = "$profile:item_heights_progress.txt";

    private static const string CFG_VEHICLES  = "CfgVehicles";
    private static const string CFG_WEAPONS   = "CfgWeapons";
    private static const string CFG_MAGAZINES = "CfgMagazines";

    //--- Applied to CfgVehicles ONLY - see PassesKindFilter. Inventory_Base covers everything in that
    //--- tree that can sit in an inventory or on the ground as loot: Clothing_Base, ItemOptics,
    //--- Container_Base, Edible_Base, Trap_Base and ExplosivesBase are all under it. It keeps out
    //--- House / HouseNoDestruct (buildings) and Man, which are not what loot point heights are for.
    private static const string KIND_FILTER = "Inventory_Base";

    //--- Vanilla's own fallback for a model with no collision geometry - see hologram.c
    //--- GetProjectionCollisionBox(). These are the authored placement bounds and are a great deal
    //--- closer to the truth than ClippingInfo's render bounds.
    private static const string MEMPOINT_MIN = "box_placing_min";
    private static const string MEMPOINT_MAX = "box_placing_max";

    private static const int   ITEMS_PER_FRAME = 25;

    //--- Let the world settle before spawning anything.
    private static const float START_DELAY_S   = 5.0;

    //--- Flip to true to log every single classname. Not needed for crash attribution any more (the
    //--- progress file does that), but handy when something goes wrong in a way that does not crash.
    private static const bool  LOG_EVERY_ITEM  = false;

    private ref array<string> m_Trees;

    private FileHandle m_File;
    private bool  m_FileOpen;
    private bool  m_Armed;
    private float m_Delay;
    private int   m_TreeIndex;
    private int   m_Index;
    private int   m_Count;

    private int   m_Dumped;
    private int   m_SkippedScope;
    private int   m_SkippedKind;
    private int   m_SkippedCreate;

    void ~DumpItemHeights()
    {
        Close();
    }

    //------------------------------------------------------------------------------------------------

    void Arm()
    {
        //--- Gate 3 of 3. IsMissionOffline() is Community-Online-Tools', not vanilla's;
        //--- !IsMultiplayer() is the vanilla test and is exactly "launched by LaunchOffline.bat".
        if (GetGame().IsMultiplayer())
        {
            Print("[DumpItemHeights] not an offline mission - doing nothing.");
            return;
        }

        m_Trees = new array<string>;
        m_Trees.Insert(CFG_VEHICLES);
        m_Trees.Insert(CFG_WEAPONS);
        m_Trees.Insert(CFG_MAGAZINES);

        bool has_csv = FileExist(OUTPUT_PATH);
        bool has_progress = FileExist(PROGRESS_PATH);

        //--- Once per file, as asked for. A finished run deletes its progress file, so a CSV with no
        //--- progress file beside it means "already done". Delete the CSV to force a fresh run.
        if (has_csv && !has_progress)
        {
            Print("[DumpItemHeights] " + OUTPUT_PATH + " already exists - skipping. Delete it to re-run.");
            return;
        }

        int resume_tree = -1;
        int resume_index = -1;
        bool resuming = false;

        if (has_csv && has_progress)
            resuming = ReadProgress(resume_tree, resume_index);

        //--- A progress file with no CSV is debris from an aborted run; start clean.
        if (!has_csv)
            DeleteFile(PROGRESS_PATH);

        if (resuming)
        {
            m_File = OpenFile(OUTPUT_PATH, FileMode.APPEND);
            m_TreeIndex = resume_tree;
            m_Index = resume_index + 1;
        }
        else
        {
            m_File = OpenFile(OUTPUT_PATH, FileMode.WRITE);
            m_TreeIndex = 0;
            m_Index = 0;
        }

        if (m_File == 0)
        {
            Print("[DumpItemHeights] could not open " + OUTPUT_PATH + " for writing.");
            return;
        }

        m_FileOpen = true;

        if (resuming)
            NoteCrashedClass(resume_tree, resume_index);
        else
            FPrintln(m_File, "classname,tree,scope,size_x,size_y,size_z,min_y,max_y,radius,source");

        if (m_TreeIndex >= m_Trees.Count())
        {
            Finish();
            return;
        }

        string first_tree = m_Trees.Get(m_TreeIndex);
        m_Count = GetGame().ConfigGetChildrenCount(first_tree);
        m_Delay = START_DELAY_S;
        m_Armed = true;

        Print(string.Format("[DumpItemHeights] armed - walking %1 (%2 children) from index %3 in %4s.", first_tree, m_Count, m_Index, START_DELAY_S));
    }

    void Update(float timeslice)
    {
        if (!m_Armed)
            return;

        if (m_Delay > 0)
        {
            m_Delay = m_Delay - timeslice;
            return;
        }

        int processed = 0;
        while (processed < ITEMS_PER_FRAME)
        {
            if (m_TreeIndex >= m_Trees.Count())
                break;

            //--- Bounded: AdvanceTree always moves m_TreeIndex on, and there are only three trees.
            if (m_Index >= m_Count)
            {
                AdvanceTree();
                continue;
            }

            MeasureOne(m_TreeIndex, m_Index);
            m_Index++;
            processed++;
        }

        if (!LOG_EVERY_ITEM)
            LogProgressLine();

        if (m_TreeIndex >= m_Trees.Count())
            Finish();
    }

    //! Safe to call at any time; used by the destructor and by OnMissionFinish so a mission that
    //! ends mid-dump cannot leave the handle open.
    void Close()
    {
        if (!m_FileOpen)
            return;

        CloseFile(m_File);
        m_FileOpen = false;
        m_Armed = false;
    }

    //------------------------------------------------------------------------------------------------

    private void AdvanceTree()
    {
        string finished = m_Trees.Get(m_TreeIndex);
        Print(string.Format("[DumpItemHeights] finished %1 - dumped=%2 so far.", finished, m_Dumped));

        m_TreeIndex++;
        m_Index = 0;
        m_Count = 0;

        if (m_TreeIndex >= m_Trees.Count())
            return;

        string next = m_Trees.Get(m_TreeIndex);
        m_Count = GetGame().ConfigGetChildrenCount(next);

        Print(string.Format("[DumpItemHeights] walking %1 - %2 children.", next, m_Count));
    }

    private void LogProgressLine()
    {
        if (m_TreeIndex >= m_Trees.Count())
            return;

        string tree = m_Trees.Get(m_TreeIndex);
        Print(string.Format("[DumpItemHeights] %1 i=%2/%3 dumped=%4", tree, m_Index, m_Count, m_Dumped));
    }

    //! IsKindOf resolves against CfgVehicles only (it walks ConfigGetFullPath("CfgVehicles " + name)),
    //! so it is meaningless for the other two trees and would reject everything in them. Nothing in
    //! CfgWeapons or CfgMagazines needs filtering anyway - it is all loot by definition.
    private bool PassesKindFilter(string tree, string classname)
    {
        if (tree != CFG_VEHICLES)
            return true;

        return GetGame().IsKindOf(classname, KIND_FILTER);
    }

    private void MeasureOne(int tree_index, int index)
    {
        string tree = m_Trees.Get(tree_index);

        string classname;
        if (!GetGame().ConfigGetChildName(tree, index, classname))
            return;

        if (classname == "")
            return;

        //--- scope >= 2 only. scope 0 is abstract, and scope 1 is "inheritable but not usable on its
        //--- own" - which in practice means base classes that inherit model="" and HARD-CRASH the
        //--- engine on CreateObjectEx. ItemOptics is the measured example. Every item that actually
        //--- spawns as loot is scope 2, in all three trees.
        int scope = GetGame().ConfigGetInt(tree + " " + classname + " scope");
        if (scope < 2)
        {
            m_SkippedScope++;
            return;
        }

        if (!PassesKindFilter(tree, classname))
        {
            m_SkippedKind++;
            return;
        }

        if (LOG_EVERY_ITEM)
            Print(string.Format("[DumpItemHeights] %1 i=%2 %3", tree, index, classname));

        //--- The breadcrumb. If the create below takes the client down, this is what lets the next
        //--- launch resume past it instead of dying on the same class forever.
        WriteProgress(tree_index, index);

        //--- ECE_CREATEPHYSICS is load bearing: without a collision envelope GetCollisionBox returns
        //--- false and every row silently degrades to the looser ClippingInfo render bounds.
        //--- ECE_TRACE and ECE_UPDATEPATHGRAPH are deliberately absent - the box is measured in model
        //--- space so the spawn position is irrelevant, and vanilla's own preview spawner carries a
        //--- literal "Don't use ECE_UPDATEPATHGRAPH" warning (scriptconsoleitemstab.c).
        vector test_pos = "0 1000 0";
        Object obj = GetGame().CreateObjectEx(classname, test_pos, ECE_LOCAL | ECE_CREATEPHYSICS);
        if (!obj)
        {
            m_SkippedCreate++;
            return;
        }

        //--- Vanilla's preview spawner also calls dBodyDestroy here. It is NOT copied: that object
        //--- lives for many frames, this one is deleted before the frame ends, and destroying the
        //--- physics body risks taking away the very collision data being read below.
        EntityAI entity = EntityAI.Cast(obj);
        if (entity)
            entity.DisableSimulation(true);

        vector min_max[2];
        string source = "";

        if (obj.GetCollisionBox(min_max))
        {
            source = "collision";
        }
        else if (obj.MemoryPointExists(MEMPOINT_MIN) && obj.MemoryPointExists(MEMPOINT_MAX))
        {
            min_max[0] = obj.GetMemoryPointPos(MEMPOINT_MIN);
            min_max[1] = obj.GetMemoryPointPos(MEMPOINT_MAX);
            source = "memorypoint";
        }
        else
        {
            //--- ClippingInfo is proto float and returns a radius, so it has no failure signal. It
            //--- can never be the reason a row is skipped - it is the last resort by construction.
            obj.ClippingInfo(min_max);
            source = "clipping";
        }

        float radius = obj.GetCollisionRadius();

        vector box_min = min_max[0];
        vector box_max = min_max[1];
        vector size = box_max - box_min;

        GetGame().ObjectDelete(obj);

        WriteRow(classname, tree, scope, size, box_min, box_max, radius, source);
        m_Dumped++;
    }

    private void WriteRow(string classname, string tree, int scope, vector size, vector box_min, vector box_max, float radius, string source)
    {
        //--- Every component pulled onto its own statement before it reaches a call - EnfusionScript
        //--- has been measured to read the wrong value when an indexed read shares an expression
        //--- with a call.
        float size_x = size[0];
        float size_y = size[1];
        float size_z = size[2];
        float min_y  = box_min[1];
        float max_y  = box_max[1];

        //--- Ten fields against string.Format's %9 ceiling, so the row is built in two halves.
        string head = string.Format("%1,%2,%3,%4,%5", classname, tree, scope, Fmt(size_x), Fmt(size_y));
        string tail = string.Format("%1,%2,%3,%4,%5", Fmt(size_z), Fmt(min_y), Fmt(max_y), Fmt(radius), source);

        FPrintln(m_File, head + "," + tail);
    }

    private void Finish()
    {
        string summary = string.Format("# dumped=%1 skipped_scope=%2 skipped_kind=%3 skipped_create=%4", m_Dumped, m_SkippedScope, m_SkippedKind, m_SkippedCreate);

        FPrintln(m_File, summary);
        Close();

        //--- Removing the breadcrumb is what marks the dump complete, so the next launch skips.
        DeleteFile(PROGRESS_PATH);

        Print("[DumpItemHeights] finished. " + summary);
        Print("[DumpItemHeights] wrote " + OUTPUT_PATH);
    }

    //------------------------------------------------------------------------------------------------

    private void WriteProgress(int tree_index, int index)
    {
        FileHandle progress = OpenFile(PROGRESS_PATH, FileMode.WRITE);
        if (progress == 0)
            return;

        FPrintln(progress, string.Format("%1", tree_index));
        FPrintln(progress, string.Format("%1", index));
        CloseFile(progress);
    }

    private bool ReadProgress(out int tree_index, out int index)
    {
        tree_index = -1;
        index = -1;

        FileHandle progress = OpenFile(PROGRESS_PATH, FileMode.READ);
        if (progress == 0)
            return false;

        string line_tree = "";
        string line_index = "";

        int read_tree = FGets(progress, line_tree);
        int read_index = FGets(progress, line_index);
        CloseFile(progress);

        if (read_tree <= 0)
            return false;

        if (read_index <= 0)
            return false;

        tree_index = line_tree.ToInt();
        index = line_index.ToInt();

        if (tree_index < 0)
            return false;

        return true;
    }

    //! The class recorded in the progress file is the one that was mid-creation when the client
    //! died. Name it in both the log and the CSV so the artifact documents its own gaps.
    private void NoteCrashedClass(int tree_index, int index)
    {
        if (tree_index >= m_Trees.Count())
            return;

        string tree = m_Trees.Get(tree_index);

        string crashed = "";
        GetGame().ConfigGetChildName(tree, index, crashed);

        string note = string.Format("# resumed - %1 index %2 (%3) crashed the previous run and is skipped", tree, index, crashed);

        FPrintln(m_File, note);
        Print("[DumpItemHeights] " + note);
    }

    //! Millimetre precision is plenty for a point height and keeps the CSV readable.
    private string Fmt(float value)
    {
        float rounded = Math.Round(value * 1000.0);
        rounded = rounded / 1000.0;
        return string.Format("%1", rounded);
    }
}

#endif // DIAG_DEVELOPER
#endif // !SERVER
