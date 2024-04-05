modded class PlayerBase
{
    //credit to wardog for the quick fix for client localplayers grabbing
    private static autoptr array<PlayerBase> s_LocalPlayers = new array<PlayerBase>();

    float health_percent = -1;
    float blood_percent = -1;

#ifndef SERVER
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
#endif

    static void GetLocalPlayers(out array<PlayerBase> players)
    {
#ifndef SERVER
        players = new array<PlayerBase>();
        players.Copy(s_LocalPlayers);
#endif
    }

    void DisableInput(bool disabled)
    {
        Print(" Call To Disable (" + disabled + ") Player Input ");
        HumanInputController controller = GetInputController();

        controller.OverrideMovementSpeed( disabled, 0 );
        controller.OverrideMovementAngle( disabled, 0 );
        controller.OverrideMeleeEvade( disabled, false );
        controller.OverrideRaise( disabled, false );
        controller.OverrideMovementAngle( disabled, 0 );
    }

    bool UpdateHealthStats(float hp, float blood)
    {
        bool changed = (health_percent != hp) || (blood_percent != blood);
        health_percent = hp;
        blood_percent = hp;
        return changed;
    }

	override void CheckLiftWeapon()
	{
		// lift weapon check
		if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT)
		{
			Weapon_Base weap;
			if (Weapon_Base.CastTo(weap, GetItemInHands()))
			{
				bool limited = weap.LiftWeaponCheck(this);

				if (limited && !m_LiftWeapon_player)
					SendLiftWeaponSync(false);
				else if (!limited && m_LiftWeapon_player)
					SendLiftWeaponSync(false);
			}
			else if (m_LiftWeapon_player)
			{
				SendLiftWeaponSync(false);
			}
		}
	}

#ifdef SERVER
	int br_position = -1;

	void SetBRPosition( int position )
	{
		br_position = position;
	}

	int GetBRPosition()
	{
		return br_position;
	}
#endif
}
