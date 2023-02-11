modded class ItemBase
{
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
                    if(!GetBR().IsDebug())
                    {
                        //not in debug! collect event!
                        string itemtype = this.GetDisplayName();
                        vector pos = nplayer.GetPosition();
                        int time = GetGame().GetTime();
                        string playerid = nplayer.GetIdentity().GetPlainId();

						 GetBR().GetMatchData().LootPickedUp( playerid, itemtype, pos, time );
                    }
                }
            }
        }
    }

    override void OnInventoryExit(Man player) {
        super.OnInventoryExit(player);

        if(GetGame().IsMultiplayer() && GetGame().IsServer() && GetBR())
        {
            PlayerBase nplayer;
            if ( PlayerBase.CastTo(nplayer, player) )
            {
                //! server - process loot pick up
                if(nplayer.GetIdentity())
                {
                    if(!GetBR().IsDebug())
                    {
                        //not in debug! collect event!

                        string itemtype = this.GetDisplayName();
                        vector pos = nplayer.GetPosition();
                        int time = GetGame().GetTime();
                        string playerid = nplayer.GetIdentity().GetPlainId();

						 GetBR().GetMatchData().LootDropped( playerid, itemtype, pos, time );

                    }
                }
            }
        }
    }
}
