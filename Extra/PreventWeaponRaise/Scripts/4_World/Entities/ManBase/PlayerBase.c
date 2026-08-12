/**
 *  PreventWeaponRaise - the client-side lift suppression, plus the flag that gates it.
 *
 *  No guard: this file compiles on both sides on purpose. The client is the only side that ever
 *  lifts a weapon, but the admin switch lives in serverDZ.cfg and only the server can read that -
 *  so the server owns the flag and mirrors it down as a netsync bool. Compiling the override
 *  server side too costs nothing: vanilla's whole CheckLiftWeapon body is inside an
 *  INSTANCETYPE_CLIENT branch, so both paths below are no-ops there.
 */
modded class PlayerBase
{
    //--- Netsynced. Registered in Init() below, so the registration order is identical on both
    //--- sides - which it must be, since both sides compile this same file.
    //--- Defaults to false = suppression active, so a client that has not received its first sync
    //--- yet behaves exactly as this addon always has.
    protected bool m_VigridPreventWeaponRaiseDisabled;

    override void Init()
    {
        super.Init();

        RegisterNetSyncVariableBool("m_VigridPreventWeaponRaiseDisabled");
    }

#ifdef SERVER
    override void OnConnect()
    {
        super.OnConnect();

        VigridPreventWeaponRaise_Refresh();
    }

    override void OnReconnect()
    {
        super.OnReconnect();

        //--- A reconnect reuses the existing entity, so the flag is already right server side and
        //--- this would no-op. Push it anyway: it costs one bool and it means a rejoining player
        //--- can never end up on the wrong behaviour because a sync was missed.
        VigridPreventWeaponRaise_Refresh();
    }

    /**
     *  Server side only. Re-assert the admin's choice on this player, syncing unconditionally.
     */
    void VigridPreventWeaponRaise_Refresh()
    {
        m_VigridPreventWeaponRaiseDisabled = VigridPreventWeaponRaiseState.IsDisabled();
        SetSynchDirty();
    }
#endif

    bool VigridPreventWeaponRaise_IsDisabled()
    {
        return m_VigridPreventWeaponRaiseDisabled;
    }

    /**
     *  Vanilla raycasts ahead of the muzzle every frame and lifts the weapon when it would clip
     *  geometry. That reads as unresponsive in the close-quarters fights this mod is made of, so the
     *  raycast is discarded and only the un-lift path is kept.
     *
     *  Called every frame from HandleWeapons, inside the command handler - so nothing expensive
     *  belongs here. Reading the netsynced flag is a field access; reading serverDZ.cfg would not be.
     */
    override void CheckLiftWeapon()
    {
        //--- Admin opted out: hand the whole thing back to vanilla.
        if (m_VigridPreventWeaponRaiseDisabled)
        {
            super.CheckLiftWeapon();
            return;
        }

        // lift weapon check
        if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT)
        {
            // Never lift weapon and sync it to false if already lifted
            if (m_LiftWeapon_player)
            {
                SendLiftWeaponSync(false);
            }
        }
    }
}
