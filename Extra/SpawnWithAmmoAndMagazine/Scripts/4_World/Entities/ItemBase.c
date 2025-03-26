#ifdef SERVER
modded class ItemBase
{
	// Thanks to @incinatus for this
	override void EEOnCECreate()
	{
		super.EEOnCECreate();

		if( IsWeapon() )
		{
//			if( Flaregun.Cast(this) ) return;  // Skip flaregun

			Weapon weapon = Weapon.Cast(this);
			string magazineType = weapon.GetRandomMagazineTypeName(0);
			if( magazineType != string.Empty )
			{
				for( int i = 0; i < Math.RandomIntInclusive(1, 2); i++ )
				{
					magazineType = weapon.GetRandomMagazineTypeName(0);
					Print("Magazine type: " + magazineType);
					Print(GetGame().CreateObjectEx( magazineType, GetPosition(), ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH ));
				}
			}
			else
			{
				for( int j = 0; j < Math.RandomIntInclusive(1, 2); j++ )
				{
					array<string> ammoTypes = new array<string>;
					ConfigGetTextArray("chamberableFrom", ammoTypes);
					if( ammoTypes.Count() > 0 )
					{
						string ammoType = ammoTypes.GetRandomElement();
						EntityAI ammoPile = GetGame().CreateObjectEx( ammoType, GetPosition(), ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH );
						if( ammoPile.GetEconomyProfile() == NULL || ammoPile.GetEconomyProfile().GetNominal() == 0 )
						{
							// Ammo pile has no economy profile, delete it
							// We don't try to spawn one more to avoid infinite loop
							ammoPile.Delete();
						}
					}
//					string ammoType = weapon.GetRandomChamberableAmmoTypeName(0);
//
//					string ammoTypeName;
//					GetGame().ConfigGetText( string.Format("CfgAmmo %1 spawnPileType", ammoType), ammoTypeName );  // Get the ammo pile type
//					// GetEconomyProfile
//					if( ammoTypeName != string.Empty )
//					{
//						EntityAI ammoPile = GetGame().CreateObjectEx( ammoTypeName, GetPosition(), ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH );
//						if( ammoPile.GetEconomyProfile() == NULL || ammoPile.GetEconomyProfile().GetNominal() == 0 )
//						{
//							// Ammo pile has no economy profile, delete it
//							// We don't try to spawn one more to avoid infinite loop
//							ammoPile.Delete();
//						} else {
//							Print("Ammo type name: " + ammoTypeName);
//							Print(ammoPile.GetEconomyProfile().GetNominal());
//						}
//					}
				}
			}
		}
	}
}