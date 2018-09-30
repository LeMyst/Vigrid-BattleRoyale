class CfgPatches
{
	class DayZBR_Assets
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data"};
	};
};

class CfgVehicles {
	
	class HouseNoDestruct;
	class Land_Br_Wall: HouseNoDestruct
	{
		scope = 1;
		model = "\dayzbr\assets\p3ds\BR_Wall.p3d";
	};
	
};