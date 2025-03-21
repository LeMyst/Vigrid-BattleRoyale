#ifdef SERVER
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		// Chamber the weapon with a random bullet by default
		FillChamber( "", WeaponWithAmmoFlags.CHAMBER );

		Magazine mag = GetMagazine(0);
		if ( mag == NULL )  // Can't spawn a magazine if there's already one
		{
			SpawnAttachedMagazine( "", WeaponWithAmmoFlags.CHAMBER | WeaponWithAmmoFlags.QUANTITY_RNG );
		}
	}
}
