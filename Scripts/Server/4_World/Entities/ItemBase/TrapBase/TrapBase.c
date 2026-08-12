#ifdef SERVER
/**
 *  Remember who armed a trap, so its kills can be credited to them.
 *
 *  The twin of ExplosivesBase.c - see that file for why the activator is stored as strings rather
 *  than as an object reference. Written twice because vanilla's TrapBase and ExplosivesBase share no
 *  ancestor below ItemBase; modding at TrapBase covers LandMineTrap and every other trap, where the
 *  previous version of this file covered only the landmine.
 */
modded class TrapBase
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

    //! Never clears an existing activator - see ExplosivesBase.BR_SetActivator.
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

    override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
    {
        super.OnPlacementComplete(player, position, orientation);

        BR_SetActivator(player);
    }

    //! `killer` is the WEAPON for every shot, so this resolves through the shared resolver - the bare
    //! PlayerBase cast it replaces could never succeed.
    override void EEKilled(Object killer)
    {
        BR_SetActivator(BattleRoyaleKillAttribution.ResolvePlayerSource(killer));

        super.EEKilled(killer);
    }
}
#endif
