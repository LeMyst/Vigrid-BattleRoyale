modded class Weapon_Base {

    override void EEFired (int muzzleType, int mode, string ammoType) 
    {
        if(GetGame().IsServer() && GetBR())
        {
            //! server - log zombie kill event!
            BattleRoyaleDebug m_Debug;
            BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( GetBR() ).GetCurrentState();
            if(!Class.CastTo(m_Debug, m_CurrentState))
            {
  
                PlayerBase shooter = PlayerBase.Cast( this.GetHierarchyRootPlayer() );
                if(shooter && shooter.GetIdentity())
                {
                    string playerid = shooter.GetIdentity().GetPlainId();
                    BattleRoyaleServer.Cast(  GetBR() ).GetMatchData().Shoot( playerid, shooter.GetPosition(), GetGame().GetTime() );
                }
            }
        }
        
        super.EEFired(muzzleType, mode, ammoType);
    }
}