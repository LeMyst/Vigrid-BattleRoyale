#ifdef CHAMBER_WEAPON
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		if (GetGame().IsServer())
		{
		    FillChamber( "", WeaponWithAmmoFlags.CHAMBER );
		}
	}
}
#endif
