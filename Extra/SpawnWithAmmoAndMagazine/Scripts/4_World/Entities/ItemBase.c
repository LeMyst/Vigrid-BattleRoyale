#ifdef SERVER
modded class ItemBase
{
	// Thanks to @incinatus for this
	override void EEOnCECreate()
	{
		super.EEOnCECreate();

		//  Cheapest test first. This method runs for every item the Central Economy creates - 33,921
		//  of them on a cold ChernarusPlus boot - and only weapons need any of what follows. The
		//  config read used to sit above this test, so every item paid for it; see
		//  VigridSpawnAmmoConfig for why that mattered.
		if( !IsWeapon() ) return;
		if( VigridSpawnAmmoConfig.IsDisabled() ) return;
//		if( Flaregun.Cast(this) ) return;  // Skip flaregun

		int min_spawn = VigridSpawnAmmoConfig.GetMinSpawn();
		int max_spawn = VigridSpawnAmmoConfig.GetMaxSpawn();

		Weapon weapon = Weapon.Cast(this);
		if( weapon == NULL ) return;

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
					BattleRoyaleUtils.Warn("[SpawnWithAmmo] Skipping high capacity magazine: " + magazineType + " (" + maxMagCapacity + " rounds)");
					continue; // Try another magazine type without counting it as spawned
				}

				//  Deliberately not logged. This is the hot path: it ran 3,172 times on one boot and
				//  the two Print() calls it used to carry ("Magazine type: X", plus the object handed
				//  straight to Print) were 6,344 of the 44,700 lines in the script log. The rare
				//  diagnostics above and below are kept - they fire a handful of times, not per item.
				GetGame().CreateObjectEx(magazineType, GetPosition(), ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH);
				validMagazinesSpawned++;
			}

			if (validMagazinesSpawned < spawnCount)
			{
				BattleRoyaleUtils.Warn("[SpawnWithAmmo] Could only spawn " + validMagazinesSpawned + " valid magazines after " + attempts + " attempts");
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
					//  CreateObjectEx answers NULL for a class it cannot spawn, and the two calls
					//  below would then dereference it once per failure, per item.
					if( ammoPile == NULL ) continue;
					if( ammoPile.GetEconomyProfile() == NULL || ammoPile.GetEconomyProfile().GetNominal() == 0 )
					{
						// Ammo pile has no economy profile, delete it
						// We don't try to spawn one more to avoid infinite loop
						ammoPile.Delete();
					}
				}
			}
		}
	}
}
#endif
