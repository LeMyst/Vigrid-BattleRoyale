modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		if (GetGame().IsServer())
		{
            SpawnAmmo( "", WeaponWithAmmoFlags.CHAMBER | WeaponWithAmmoFlags.MAX_CAPACITY_MAG );
		}
	}
}
