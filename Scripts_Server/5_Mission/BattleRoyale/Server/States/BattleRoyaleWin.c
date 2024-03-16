#ifdef SERVER
class BattleRoyaleWin: BattleRoyaleState
{
    int i_SecondsTillKick;
    PlayerBase winner;
    bool complete;

    protected ref Timer m_KickTimer;

    void BattleRoyaleWin()
    {
        i_SecondsTillKick = 15; //TODO: config this
        complete = false;
    }

    //TODO: state functionality for winners!
    //TODO: Add party support
	override void Activate()
	{
		super.Activate();
	
		string winner_name = "<NO:WINNER>";
		if(GetPlayers().Count() > 0)
		{
			for ( int k = 0; k < GetPlayers().Count(); k++ )
			{
				winner = GetPlayers()[k];
				PlayerIdentity identity = winner.GetIdentity();
				if(identity)
				{
					winner_name = identity.GetName();
					Print("[Win State] Winner!");
					Print(identity.GetName());
					Print(identity.GetFullName());
					Print(identity.GetId());
					Print(identity.GetPlainId());
				}
				HandleWinner(winner);
			}
		}

		m_KickTimer = AddTimer(i_SecondsTillKick, this, "KickWinner", NULL, false);
	}

    override string GetName()
    {
        return DAYZBR_SM_WIN_NAME;
    }

    override void Deactivate()
    {
        m_KickTimer.Stop();
        
        super.Deactivate();
    }

    override bool IsComplete()
    {
        if(GetPlayers().Count() == 0 && IsActive())
            Deactivate();
        
        return super.IsComplete(); //go to restart state when player disconnects
    }

    void HandleWinner(PlayerBase player_winner)
    {
        MessagePlayer(player_winner, "Congratulations! You won Battle Royale!");
    }

    void KickWinner()
    {
        if(winner && winner.GetIdentity() )
        {
            RemovePlayer(winner); //disconnect does not trigger RemovePlayer !
            GetGame().DisconnectPlayer( winner.GetIdentity() );
        }

        Deactivate();
    }
}
