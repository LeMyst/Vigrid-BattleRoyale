modded class ActionGetOutTransport {

    override void OnEndServer( ActionData action_data ) 
    {
		GetOutTransportActionData got_action_data = GetOutTransportActionData.Cast(action_data);
        if(got_action_data)
        {
            PlayerBase player = action_data.m_Player;
            if(player.GetIdentity())
            {
                CarScript car;
                if ( Class.CastTo(car, got_action_data.m_Car) )
                {   

                    //TODO: register event if match is ongoing! (we have CAR and PLAYER)
                    string cartype = car.GetDisplayName();
                    string playerid =  player.GetIdentity().GetPlainId();
                    vector vehiclepos = car.GetPosition();
                    int time = GetGame().GetTime();
                    BattleRoyaleDebug m_Debug;
                    BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( GetBR() ).GetCurrentState();
                    if(!Class.CastTo(m_Debug, m_CurrentState))
                    {
                        //we are not in a debug state, therefore we assume we're in game and events need processed!
                        BattleRoyaleServer.Cast(  GetBR() ).GetMatchData().GetOutVehicle( playerid, cartype, vehiclepos, time );
                    }
                }
            }
        }
        else
        {
            Error("invalid get out action?!")
        }

        super.OnEndServer( action_data );
    }
}