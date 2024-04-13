class CfgPatches
{
	class BattleRoyale_Models_Shapes
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] =
		{
			"DZ_Data"
		};
	};
};

class CfgVehicles
{
	class HouseNoDestruct;
	class the_zone: HouseNoDestruct
	{
		model="\BattleRoyale_Models_Shapes\zone.p3d";
	};
};
