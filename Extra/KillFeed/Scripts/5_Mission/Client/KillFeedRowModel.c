#ifndef SERVER
/**
 *  KillFeed - one live row: the entry, when it expires, and the throwaway entity being rendered.
 *
 *  The preview entity hangs off the model rather than off the row widget, so that pushing a new
 *  kill only re-points SetItem at an already-built entity instead of respawning every model on
 *  screen. At most KILLFEED_MAX_ROWS of these exist at once.
 */
class KillFeedRowModel
{
    ref KillFeedEntry entry;
    int expires_at;

    //--- Not a ref: an engine-owned Object, released with Delete() rather than by refcount.
    EntityAI preview;

    void KillFeedRowModel(KillFeedEntry source_entry)
    {
        entry = source_entry;
        expires_at = GetGame().GetTime() + (KILLFEED_ROW_SECONDS * 1000);
        preview = SpawnPreview(source_entry);
    }

    void ~KillFeedRowModel()
    {
        Release();
    }

    //! Every removal path calls this explicitly; the destructor is only a safety net.
    void Release()
    {
        if (!preview)
            return;

        preview.Delete();
        preview = NULL;
    }

    /**
     *  Build the thing the ItemPreviewWidget renders: a client-local weapon wearing the same
     *  accessories the killer had. ECE_LOCAL keeps it off the network and out of the central
     *  economy; simulation is switched off so it cannot tick, fall or take damage.
     *
     *  This is vanilla's own preview-spawn recipe, as used by the script console's item tab.
     */
    private EntityAI SpawnPreview(KillFeedEntry source_entry)
    {
        if (!source_entry)
            return NULL;
        if (!source_entry.HasWeapon())
            return NULL;

        //--- Vanilla brackets this call with DayZGame.m_IsPreviewSpawn, but that field and its only
        //--- consumer both live behind #ifdef DEVELOPER, so it does nothing in a shipped build and
        //--- referencing it fails to compile.
        EntityAI weapon = EntityAI.Cast(GetGame().CreateObjectEx(source_entry.weapon_type, "0 0 0", ECE_LOCAL));

        if (!weapon)
        {
            KillFeedLog.Warn("Could not create preview entity for " + source_entry.weapon_type);
            return NULL;
        }

        //--- Attach before disabling simulation: the accessories are what make this feature worth
        //--- having, so they go on while the entity is still fully alive.
        GameInventory inventory = weapon.GetInventory();
        if (inventory)
        {
            array<string> types = source_entry.GetAttachmentTypes();
            int count = types.Count();
            for (int i = 0; i < count; i++)
            {
                string type = types.Get(i);
                if (type == "")
                    continue;

                if (!inventory.CreateAttachment(type))
                    KillFeedLog.Debug("Could not attach " + type + " to " + source_entry.weapon_type);
            }
        }

        weapon.DisableSimulation(true);
        weapon.SetAllowDamage(false);

        return weapon;
    }
}
#endif
