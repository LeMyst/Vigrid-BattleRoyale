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
}
