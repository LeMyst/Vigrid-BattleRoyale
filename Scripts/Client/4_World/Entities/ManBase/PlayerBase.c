modded class PlayerBase
{
#ifndef SERVER
    //credit to wardog for the quick fix for client localplayers grabbing
    private static autoptr array<PlayerBase> s_LocalPlayers = new array<PlayerBase>();

    void PlayerBase()
    {
        if (s_LocalPlayers)
        {
            s_LocalPlayers.Insert(this);
        }
    }

    void ~PlayerBase()
    {
        if (s_LocalPlayers)
        {
            int localIndex = s_LocalPlayers.Find(this);
            if (localIndex >= 0)
            {
                s_LocalPlayers.Remove(localIndex);
            }
        }
    }

    static void GetLocalPlayers(out array<PlayerBase> players)
    {
        players = new array<PlayerBase>();
        players.Copy(s_LocalPlayers);
    }
#endif

    void DisableInput(bool disabled)
    {
    	if ( disabled )
        	BattleRoyaleUtils.Trace( "Call To Disable Player Input" );
		else
			BattleRoyaleUtils.Trace( "Call To Enable Player Input" );

		//--- Voice is deliberately NOT touched here. This method runs on the client for states 2 and
		//--- 3 (driven by the SetInput RPC) where MuteAllPlayers/EnableVoN are no-ops, because the
		//--- VON router only answers to the server - so the gag this used to attempt never happened.
		//--- On the server, in state 4, it did work, but it gagged everyone globally and would defeat
		//--- party voice during prepare. BattleRoyaleVoice is now the single owner of voice policy.

		HumanInputControllerOverrideType override_type = HumanInputControllerOverrideType.DISABLED;
		if ( disabled )
		{
			// disabled means we want to enable the override
			override_type = HumanInputControllerOverrideType.ENABLED;
		}

        SetSynchDirty();

        HumanInputController hic = GetInputController();
        if ( hic )
		{
			hic.OverrideMovementSpeed( override_type, 0 );
			hic.OverrideMovementAngle( override_type, 0 );
			hic.OverrideMeleeEvade( override_type, false );
			hic.OverrideRaise( override_type, false );
			hic.OverrideFreeLook( override_type, false );
			hic.SetDisabled( disabled );
		}
    }
}
