#ifdef SERVER
/**
 *  Remember who armed an explosive, so its kills can be credited to them.
 *
 *  Modded at ExplosivesBase rather than at each concrete class: vanilla parents Grenade_Base,
 *  ClaymoreMine, ImprovisedExplosive and Plastic_Explosive off it, and only the first of those used
 *  to record anything - so every placed charge in the game credited its classname instead of its
 *  owner. TrapBase gets the same treatment in TrapBase.c; the two hierarchies are unrelated in
 *  vanilla, which is why this is written twice rather than shared.
 *
 *  Both fields are plain strings ON PURPOSE. An object reference would go stale the moment the
 *  owner died or disconnected, which is precisely the window a thrown grenade or an armed trap
 *  lives in - the whole point of the feature is that the kill outlives its owner.
 */
modded class ExplosivesBase
{
    protected string m_ActivatorId = "";
    protected string m_ActivatorName = "";

    string GetActivatorId()
    {
        return m_ActivatorId;
    }

    string GetActivatorName()
    {
        return m_ActivatorName;
    }

    /**
     *  Record `player` as responsible for whatever this device does next.
     *
     *  Never clears an existing activator: a device that cannot resolve a new one keeps the one it
     *  had, so an unrelated event touching a live grenade cannot orphan the thrower's attribution.
     */
    void BR_SetActivator(Man player)
    {
        PlayerBase player_base = PlayerBase.Cast(player);
        if (!player_base)
            return;

        string uid = player_base.player_steamid;
        if (uid == "" && player_base.GetIdentity())
            uid = player_base.GetIdentity().GetPlainId();

        if (uid == "")
            return;

        m_ActivatorId = uid;
        m_ActivatorName = BattleRoyaleKillAttribution.NameOfPlayer(player_base);

        BattleRoyaleUtils.Trace("[Kills] armed " + GetType() + " by " + uid);
    }

    /**
     *  Placement AND arming both land here.
     *
     *  ActionArmExplosive.OnFinishProgressServer calls OnPlacementComplete(action_data.m_Player, ...)
     *  when a charge is armed in the world, so this one override covers the remote-detonated devices
     *  that are never "placed" in the deployment sense.
     */
    override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
    {
        super.OnPlacementComplete(player, position, orientation);

        BR_SetActivator(player);
    }

    /**
     *  Someone destroyed the device and set it off.
     *
     *  Resolved through the shared resolver rather than a bare cast: `killer` is the WEAPON for every
     *  shot, so the cast this replaced could never succeed and shooting an armed charge silently kept
     *  crediting whoever placed it.
     */
    override void EEKilled(Object killer)
    {
        //--- Before super: it calls InitiateExplosion, and the victim's own EEKilled reads the
        //--- activator back off this object.
        BR_SetActivator(BattleRoyaleKillAttribution.ResolvePlayerSource(killer));

        super.EEKilled(killer);
    }
}
#endif
