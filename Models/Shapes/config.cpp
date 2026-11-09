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

	//--- One flat 2 x 0.25 m quad of the on-ground zone ring, spawned locally on each client and
	//--- recoloured per circle (BattleRoyaleGroundRing). scope=1 like vanilla's
	//--- StaticObj_fireplace_spot_snow: spawned by script, never offered to admin spawners.
	class BR_GroundSegment: HouseNoDestruct
	{
		scope = 1;
		model = "\Vigrid-BattleRoyale\Models\Shapes\ground_segment.p3d";
		hiddenSelections[] = {"camo"};
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(0.05,0.25,1,0.5,ca)"};
	};
};
