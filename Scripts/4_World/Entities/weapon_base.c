modded class Weapon_Base {

    override void EEFired (int muzzleType, int mode, string ammoType) 
    {
        if(GetGame().IsMultiplayer() && GetGame().IsServer() && GetBR())
        {
            //! server - log zombie kill event!
            if(!GetBR().IsDebug())
            {
  
                PlayerBase shooter = PlayerBase.Cast( this.GetHierarchyRootPlayer() );
                if(shooter && shooter.GetIdentity())
                {
                    string playerid = shooter.GetIdentity().GetPlainId();
                    GetBR().GetMatchData().Shoot( playerid, shooter.GetPosition(), GetGame().GetTime() );
                }
            }
        }
        
        super.EEFired(muzzleType, mode, ammoType);
    }
}