modded class PlayerBase
{
	override void CheckLiftWeapon()
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
}
