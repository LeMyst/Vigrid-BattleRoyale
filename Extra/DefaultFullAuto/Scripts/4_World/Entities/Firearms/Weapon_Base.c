#ifdef SERVER
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();

		// Find the auto fire mode for each muzzle and set it as the current mode
		for (int i = 0; i < GetMuzzleCount(); i++)
		{
			int newMuzzleMode = GetCurrentMode(i);
			for (int j = 0; j < GetMuzzleModeCount(i); j++)
			{
				SetCurrentMode(i, j);
				if (GetCurrentModeAutoFire(i))
				{
					// We found the auto fire mode, override the current muzzle mode
					newMuzzleMode = j;
					break;
				}
			}
			SetCurrentMode(i, newMuzzleMode);
		}
	}
}
