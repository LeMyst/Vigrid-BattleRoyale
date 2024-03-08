#ifdef SERVER
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		FillChamber( "", WeaponWithAmmoFlags.CHAMBER );
	}
}
#endif