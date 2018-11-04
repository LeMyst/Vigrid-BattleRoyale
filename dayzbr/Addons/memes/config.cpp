class CfgPatches
{
	class DayZBR_Assets
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {};
	};
};

class CfgVehicles {
	
	class HouseNoDestruct;
	class Lightsaber_inactive_preview: HouseNoDestruct
	{
		scope = 1;
		model = "\dayzbr\memes\p3ds\Lightsaber_inactive.p3d";
	};
	class Lightsaber_active_preview: HouseNoDestruct
	{
		scope = 1;
		model = "\dayzbr\memes\p3ds\Lightsaber_active.p3d";
	};
};
