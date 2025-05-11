class CfgPatches
{
    class vigrid_disable_fog_chernarusplus
    {
        requiredAddons[]=
        {
            "DZ_Worlds_Chernarusplus_World"
        };
    };
};

class CfgWorlds
{
    class DefaultWorld;
	class CAWorld: DefaultWorld
	{
		class Weather
		{
            class VolFog
            {
                CameraFog=0;
                UseDynamic=0;

                Item1[]={0,0,0,0,0};
                Item2[]={0,0,0,0,0};
            };
        };
	};
    class ChernarusPlus: CAWorld
    {
        class Weather
        {
            class VolFog
            {
                CameraFog=0;
                UseDynamic=0;

                Item1[]={0,0,0,0,0};
                Item2[]={0,0,0,0,0};
            };
        };
    };
};