
modded class ActionGetInTransport
{


    override void OnEndServer( ActionData action_data ) 
    {
        PlayerBase player = action_data.m_Player;
        if(player.GetIdentity())
        {
            CarScript car = CarScript.Cast(action_data.m_Target.GetObject());
            if (car)
            {

                //register event with br if in match!
                string cartype = car.GetDisplayName();
                string playerid =  player.GetIdentity().GetPlainId();
                vector vehiclepos = car.GetPosition();
                int time = GetGame().GetTime();
                BattleRoyaleDebug m_Debug;
                BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( GetBR() ).GetCurrentState();
                if(!Class.CastTo(m_Debug, m_CurrentState))
                {
                    //we are not in a debug state, therefore we assume we're in game and events need processed!
                    BattleRoyaleServer.Cast(  GetBR() ).GetMatchData().GetInVehicle( playerid, cartype, vehiclepos, time );
                }
            }
        }

        super.OnEndServer( action_data );
    }

}