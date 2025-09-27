#ifndef SERVER
modded class PlayerBase
{
	override void CheckLiftWeapon()
	{
		bool config_disable_prevent_weapon_raise = GetGame().ServerConfigGetInt( "BRDisablePreventWeaponRaise" );

		if (!config_disable_prevent_weapon_raise)
		{
			// lift weapon check
			if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT)
			{
				// Never lift weapon and sync it to false if already lifted
				if (m_LiftWeapon_player)
				{
					SendLiftWeaponSync(false);
				}
			}
		}
		else
		{
			super.CheckLiftWeapon();
		}
	}
}
