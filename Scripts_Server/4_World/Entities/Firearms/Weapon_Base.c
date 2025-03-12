#ifdef SERVER
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		// Chamber the weapon with a random bullet by default
		FillChamber( "", WeaponWithAmmoFlags.CHAMBER );
	}
}
