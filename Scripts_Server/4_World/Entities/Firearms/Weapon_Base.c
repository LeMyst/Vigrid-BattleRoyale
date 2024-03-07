#ifdef SERVER
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		//SpawnAmmo( "", WeaponWithAmmoFlags.CHAMBER | WeaponWithAmmoFlags.MAX_CAPACITY_MAG );
	}
}
#endif