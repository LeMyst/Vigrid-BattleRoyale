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
	class BR_Zone: HouseNoDestruct
	{
		scope = 2;
		model = "\Vigrid-BattleRoyale\Models\Shapes\zone.p3d";
	};
	class Basic_Zone: HouseNoDestruct
	{
		scope = 2;
		model = "\Vigrid-BattleRoyale\Models\Shapes\basic_zone.p3d";
	};
};