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

#ifdef SPECTATOR
    float health_percent = -1;
    float blood_percent = -1;

    bool UpdateHealthStats(float hp, float blood)
    {
        bool changed = (health_percent != hp) || (blood_percent != blood);
        health_percent = hp;
        blood_percent = hp;
        return changed;
    }
#endif
#endif

	void LockControls(bool state)
	{
		if (state == true)  // Lock
		{
			GetGame().GetInput().ChangeGameFocus(1, INPUT_DEVICE_MOUSE);
			GetGame().GetInput().ChangeGameFocus(1, INPUT_DEVICE_KEYBOARD);
			GetGame().GetInput().ChangeGameFocus(1, INPUT_DEVICE_GAMEPAD);

			if (GetGame().GetUIManager())
			{
				GetGame().GetUIManager().ShowUICursor(true);
				if (GetGame().GetUIManager().IsDialogVisible())
					GetGame().GetUIManager().CloseDialog();
			}
		}
		else  // Unlock
		{
			GetGame().GetInput().ChangeGameFocus(-1, INPUT_DEVICE_MOUSE);
			GetGame().GetInput().ChangeGameFocus(-1, INPUT_DEVICE_KEYBOARD);
			GetGame().GetInput().ChangeGameFocus(-1, INPUT_DEVICE_GAMEPAD);

			if (GetGame().GetUIManager())
			{
				if (GetGame().GetUIManager().GetMenu())
				{
					GetGame().GetUIManager().ShowUICursor(true);
				}
				else
				{
					GetGame().GetUIManager().ShowUICursor(false);
				}
			}
		}

		GetGame().MuteAllPlayers( GetIdentity().GetPlainId() , state );  // Mute all players when controls are locked
		GetGame().EnableVoN(this, !state);  // Disable VoN when controls are locked
	}

    void DisableInput(bool disabled)
    {
    	if ( disabled )
        	BattleRoyaleUtils.Trace( "Call To Disable Player Input" );
		else
			BattleRoyaleUtils.Trace( "Call To Enable Player Input" );

		// Disable Voice over Network
		if ( disabled )
		{
			GetGame().MuteAllPlayers( GetIdentity().GetPlainId() , true );
			GetGame().EnableVoN(this, false);
		}
		else
		{
			GetGame().EnableVoN(this, true);
			GetGame().MuteAllPlayers( GetIdentity().GetPlainId() , false );
		}

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
