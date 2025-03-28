#ifdef SERVER
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		// Chamber the weapon with a random bullet by default
		FillChamber( "", WeaponWithAmmoFlags.CHAMBER );

		Magazine mag = GetMagazine(0);
		array<string> magazines = new array<string>;
		if ( mag == NULL )  // Can't spawn a magazine if there's already one
		{
			SpawnAmmo();
//			if ( GetGame().ConfigIsExisting( "cfgWeapons " + GetType() + " magazines" ) )  // Check if the weapon can have magazines
//			{
//				SpawnAttachedMagazine( "", WeaponWithAmmoFlags.CHAMBER | WeaponWithAmmoFlags.QUANTITY_RNG );
//			}
//			else
//			{
//				SpawnAmmo();
//			}
		}
	}
}
