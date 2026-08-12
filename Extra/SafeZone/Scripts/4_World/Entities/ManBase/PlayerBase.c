/**
 *  SafeZone - the damage half of the truce, plus the state that carries it to the client.
 *
 *  No guard: this file compiles on both sides on purpose. The server owns the flag and decides what
 *  damage lands; the client needs the same flag locally so WeaponManager can refuse the trigger
 *  without a round trip.
 */
modded class PlayerBase
{
    //--- Netsynced. Registered in Init() below, so the registration order is identical on both
    //--- sides - which it must be, since both sides compile this same file.
    protected bool m_VigridSafeZoneActive;

    override void Init()
    {
        super.Init();

        RegisterNetSyncVariableBool("m_VigridSafeZoneActive");
    }

#ifdef SERVER
    override void OnConnect()
    {
        super.OnConnect();

        //--- Covers the player who joins an already-running lobby: SetActive() only reached the
        //--- players who were connected at the time it was called.
        VigridSafeZone_Refresh();
    }

    override void OnReconnect()
    {
        super.OnReconnect();

        //--- A reconnect reuses the existing entity, so the flag is already right server side and
        //--- VigridSafeZone_Apply would no-op. Push it anyway: it costs nothing and it means a
        //--- rejoining player can never end up armed in a lobby because a sync was missed.
        VigridSafeZone_Refresh();
    }

    /**
     *  Server side only. Re-assert the current truce on this player, syncing unconditionally.
     */
    void VigridSafeZone_Refresh()
    {
        m_VigridSafeZoneActive = VigridSafeZoneState.IsActive();
        SetSynchDirty();
    }
#endif

    /**
     *  Server side only. Set the flag and push it to the owning client.
     */
    void VigridSafeZone_Apply(bool active)
    {
        if (m_VigridSafeZoneActive == active)
            return;

        m_VigridSafeZoneActive = active;
        SetSynchDirty();
    }

    bool VigridSafeZone_IsActive()
    {
        return m_VigridSafeZoneActive;
    }

    /**
     *  Engine event fired right before damage is applied; returning false discards the hit outright.
     *
     *  This is the same choke point DayZ Expansion uses for its safezone, but the predicate is much
     *  narrower. Expansion cancels everything - a safezone player cannot be hurt by anything at all.
     *  Here only another player's doing is cancelled, so falls, drowning, infected, animals and any
     *  scripted damage the host game applies still land exactly as they would with the addon absent.
     *
     *  Note the trade-off: a discarded hit produces no hit reaction on the victim, so a punch lands
     *  visually for the attacker but the target does not flinch. TotalDamageResult is getter-only,
     *  so scaling the damage to zero instead is not available - cancelling is the only clean route.
     */
    override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
            return false;

        if (!m_VigridSafeZoneActive)
            return true;

        if (!VigridSafeZone_IsPlayerInflicted(source))
            return true;

        return false;
    }

    /**
     *  Did another player cause this hit?
     *
     *  Scripted damage - DecreaseHealthCoef and friends - arrives with the victim as their own
     *  source, and so falls through as false. That is deliberate: swallowing it would break every
     *  host game that damages players itself.
     */
    bool VigridSafeZone_IsPlayerInflicted(EntityAI source)
    {
        if (!source)
            return false;

        //--- Bare hands: the attacker is the source entity itself.
        PlayerBase attacker = PlayerBase.Cast(source);

        //--- Firearms and melee weapons: the source is the item, the attacker is whoever holds it.
        if (!attacker)
            attacker = PlayerBase.Cast(source.GetHierarchyRootPlayer());

        if (attacker)
            return attacker != this;

        //--- Explosives have no player left in their hierarchy once thrown or placed, but nothing
        //--- else leaves one lying around either. Classified by type, the same way the host mod's
        //--- own kill reporting does it - and at the vanilla PARENTS, never at a leaf class.
        //--- IsExplosive() is declared on ItemBase and only ExplosivesBase overrides it to true, so
        //--- it covers Grenade_Base, Claymore, IED and Plastic Explosive; TrapBase sits outside that
        //--- branch entirely and needs naming separately. Naming LandMineTrap there let every other
        //--- player-placed trap - BearTrap and friends - still land damage during the truce.
        ItemBase item = ItemBase.Cast(source);
        if (item && item.IsExplosive())
            return true;

        if (source.IsInherited(TrapBase))
            return true;

        return false;
    }
}
