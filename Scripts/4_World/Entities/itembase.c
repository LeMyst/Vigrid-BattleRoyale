
modded class ItemBase {


    override void OnInventoryEnter(Man player) {
        super.OnInventoryEnter(player);

        if(GetGame().IsServer() && GetBR())
        {
            PlayerBase nplayer;
            if ( PlayerBase.CastTo(nplayer, player) )
            {
                //! server - process loot pick up   
                if(nplayer.GetIdentity())
                {
                    BattleRoyaleDebug m_Debug;
                    BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( GetBR() ).GetCurrentState();
                    if(!Class.CastTo(m_Debug, m_CurrentState))
                    {
                        //not in debug! collect event!

                        string itemtype = this.GetDisplayName();
                        vector pos = nplayer.GetPosition();
                        int time = GetGame().GetTime();
                        string playerid = nplayer.GetIdentity().GetPlainId();

						BattleRoyaleServer.Cast(  GetBR() ).GetMatchData().LootPickedUp( playerid, itemtype, pos, time );

                    }
                }
            }
        }
    }
    override void OnInventoryExit(Man player) {
        super.OnInventoryExit(player);

        if(GetGame().IsServer() && GetBR())
        {
            PlayerBase nplayer;
            if ( PlayerBase.CastTo(nplayer, player) )
            {
                //! server - process loot pick up   
                if(nplayer.GetIdentity())
                {
                    BattleRoyaleDebug m_Debug;
                    BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( GetBR() ).GetCurrentState();
                    if(!Class.CastTo(m_Debug, m_CurrentState))
                    {
                        //not in debug! collect event!

                        string itemtype = this.GetDisplayName();
                        vector pos = nplayer.GetPosition();
                        int time = GetGame().GetTime();
                        string playerid = nplayer.GetIdentity().GetPlainId();

						BattleRoyaleServer.Cast(  GetBR() ).GetMatchData().LootDropped( playerid, itemtype, pos, time );

                    }
                }
            }
        }
    }
}