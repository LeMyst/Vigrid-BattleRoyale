#ifdef SERVER
modded class ItemBase
{
	// Thanks to @incinatus for this
	override void EEOnCECreate()
	{
		super.EEOnCECreate();

		bool config_disable_spawn_with_ammo = GetGame().ServerConfigGetInt( "BRDisableSpawnWithAmmo" ) == 1;
		if( IsWeapon() && !config_disable_spawn_with_ammo )
		{
//			if( Flaregun.Cast(this) ) return;  // Skip flaregun

			int min_spawn = 1;
			int max_spawn = 2;
			int config_min_spawn = GetGame().ServerConfigGetInt( "BRMinSpawnAmmo" );
			int config_max_spawn = GetGame().ServerConfigGetInt( "BRMaxSpawnAmmo" );

			// Override min/max spawn if set in server config
			// If min is set below 0, set it to 0
			// If max is set below min, set it to min
			// If max is set to 0 or below, ignore it and use default
			if( config_min_spawn > 0 )
			{
				min_spawn = config_min_spawn;
			}
			if( config_min_spawn < 0 )
			{
				min_spawn = 0;
			}

			if( config_max_spawn > 0 )
			{
				max_spawn = config_max_spawn;
			}

			// Ensure max is not lower than min
			if( max_spawn < min_spawn )
			{
				max_spawn = min_spawn;
			}

			Weapon weapon = Weapon.Cast(this);
			string magazineType = weapon.GetRandomMagazineTypeName(0);
			// Try to spawn a magazine of the weapon's compatible magazines
			if( magazineType != string.Empty )
			{
				int spawnCount = Math.RandomIntInclusive(min_spawn, max_spawn);
				int validMagazinesSpawned = 0;
				int maxAttempts = spawnCount * 10; // Set a reasonable limit to prevent infinite loops
				int attempts = 0;

				// Keep trying until we've spawned enough valid magazines or hit the attempt limit
				while (validMagazinesSpawned < spawnCount && attempts < maxAttempts)
				{
					attempts++;
					magazineType = weapon.GetRandomMagazineTypeName(0);

					// Skip high capacity magazines (check if > 100 rounds)
					int maxMagCapacity = GetGame().ConfigGetInt("CfgMagazines " + magazineType + " count");
					if(maxMagCapacity > 100)
					{
						Print("[SpawnWithAmmo] Skipping high capacity magazine: " + magazineType + " (" + maxMagCapacity + " rounds)");
						continue; // Try another magazine type without counting it as spawned
					}

					// Valid magazine found, spawn it
					Print("Magazine type: " + magazineType);
					Print(GetGame().CreateObjectEx(magazineType, GetPosition(), ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH));
					validMagazinesSpawned++;
				}

				if (validMagazinesSpawned < spawnCount)
				{
					Print("[SpawnWithAmmo] Could only spawn " + validMagazinesSpawned + " valid magazines after " + attempts + " attempts");
				}
			}
			else  // No compatible magazines, try to spawn chamberable ammo piles instead
			{
				for( int j = 0; j < Math.RandomIntInclusive(min_spawn, max_spawn); j++ )
				{
					array<string> ammoTypes = new array<string>;
					ConfigGetTextArray("chamberableFrom", ammoTypes);
					if( ammoTypes.Count() > 0 )
					{
						string ammoType = ammoTypes.GetRandomElement();
						EntityAI ammoPile = EntityAI.Cast(GetGame().CreateObjectEx( ammoType, GetPosition(), ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH ));
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