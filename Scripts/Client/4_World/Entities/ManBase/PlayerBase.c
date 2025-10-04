modded class PlayerBase
{
	bool m_isInvisible;

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

	override void Init()
	{
		super.Init();
		RegisterNetSyncVariableBool("m_isInvisible");
	}

	void LockControls(bool state)
	{
		if (state == true)
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
		else
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

	// From VPP Admin Tools
	float GetAimingAngleLR()
	{
		HumanCommandWeapons hcw = GetCommandModifier_Weapons();
		if (hcw != null)
			return hcw.GetBaseAimingAngleLR();
		return 0.0;
	}

	// From VPP Admin Tools
	float GetAimingAngleUD()
	{
		HumanCommandWeapons hcw = GetCommandModifier_Weapons();
		if (hcw != null)
			return hcw.GetBaseAimingAngleUD();
		return 0.0;
	}
}
