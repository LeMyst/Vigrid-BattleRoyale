class BattleRoyaleLootData extends BattleRoyaleDataBase
{
    float constant_scale = 0.75;

    ref array<float> Weights = {
        1, //weapons rare
        2,
        4, //weapons common
        1, //gear rare
        2,
        4, //gear common
        1, //misc rare
        2,
        4, //misc common
        3, //food
        3, //drink
        3, //medical
        3, //ammo
        3, //grenades
        3 //attachments 
    };

    ref array<string> Weapons_Rare = { 
        "FAL",
        "SVD",
        "Engraved1911"
    };
    ref array<string> Weapons_Normal = {
        "VSS",
        "Winchester70",
        "Mosin9130",
        "SKS",
        "AKM",
        "AK74",
        "AKS74U"  
        "AK101",
        "M4A1"
    };
    ref array<string> Weapons_Common = {
        "MKII",
        "CZ75",
        "Glock19",
        "FNX45",
        "CZ61",
        "MP5K",
        "UMP45",
        "Colt1911",
        "B95",
        "Saiga",
        "Ruger1022",
        "CZ527"
    };
    
    ref array<string> Gear_Rare = {
        "SmershBag",
        "GorkaPants_Summer",
        "GorkaPants_Autumn",
        "GorkaPants_Flat",
        "GorkaPants_PautRev",
        "USMCPants_Desert",
        "USMCPants_Woodland",
        "BDUPants",
        "GorkaEJacket_Summer",
        "GorkaEJacket_Flat",
        "GorkaEJacket_Autumn",
        "GorkaEJacket_PautRev",
        "ZSh3PilotHelmet"
        "Ssh68Helmet",
        "NVGHeadstrap",
        "PlateCarrierVest",
        "SmershVest",
        "AliceBag_Camo",
        "AliceBag_Green",
        "AliceBag_Black"
    };
    ref array<string> Gear_Normal = {
        "AssaultBag_Ttsko",
        "AssaultBag_Black",
        "AssaultBag_Green",
        "MountainBag_Red",
        "MountainBag_Blue",
        "MountainBag_Orange",
        "MountainBag_Green",
        "TortillaBag",
        "TTSKOPants",
        "HuntingJacket_Autumn",
        "HuntingJacket_Brown",
        "HuntingJacket_Spring",
        "HuntingJacket_Summer",
        "HuntingJacket_Winter",
        "HunterPants_Autumn",
        "HunterPants_Brown",
        "HunterPants_Spring",
        "HunterPants_Summer",
        "HunterPants_Winter",
        "M65Jacket_Black",
        "M65Jacket_Khaki",
        "M65Jacket_Olive",
        "M65Jacket_Tan",
        "TTsKOJacket_Camo",
        "FirefighterJacket_Beige",
        "FirefighterJacket_Black",
        "FirefightersPants_Beige",
        "FirefightersPants_Black",
        "PoliceJacket",
        "PolicePants",
        "PoliceJacketOrel",
        "PolicePantsOrel",
        "ConstructionHelmet_Orange",
        "ConstructionHelmet_White",
        "ConstructionHelmet_Yellow",
        "FirefightersHelmet_Red",
        "FirefightersHelmet_White",
        "FirefightersHelmet_Yellow",
        "UKAssVest_Black",
        "UKAssVest_Camo",
        "UKAssVest_Khaki",
        "UKAssVest_Olive",
        "CoyoteBag_Brown",
        "MotoHelmet_Black",
        "MotoHelmet_Blue",
        "MotoHelmet_Green",
        "MotoHelmet_Grey",
        "MotoHelmet_Lime",
        "MotoHelmet_Red",
        "MotoHelmet_White",
        "PressVest_Blue"
    };
    ref array<string> Gear_Common = {
        "DryBag_Orange",
        "DryBag_Blue",
        "DryBag_Green",
        "DryBag_Black",
        "DryBag_Red",
        "DryBag_Yellow",
        "HuntingBag",
        "CargoPants_Beige",
        "CargoPants_Black",
        "CargoPants_Blue",
        "CargoPants_Green",
        "CargoPants_Grey",
        "NBCPantsGray",
        "Hoodie_Blue",
        "Hoodie_Black",
        "Hoodie_Brown",
        "Hoodie_Green",
        "Hoodie_Grey",
        "Hoodie_Red",
        "TacticalShirt_Grey",
        "TacticalShirt_Black",
        "TacticalShirt_Olive",
        "TacticalShirt_Tan",
        "HikingJacket_Black",
        "HikingJacket_Blue",
        "HikingJacket_Red",
        "HikingJacket_Green",
        "RidersJacket_Black"
    };

    ref array<string> Misc_Rare = {
        "PersonalRadio",
        "WoodAxe",
        "Shovel",
        "NailedBaseballBat",
        "Machete",
        "BaseballBat"
    };
    ref array<string> Misc_Normal = {
        "CanOpener",
        "Hatchet",
        "HuntingKnife",
        "PurificationTablets",
        "SewingKit",
        "DuctTape",
        "VitaminBottle"
    };
    ref array<string> Misc_Common = {
        "Rag",
        "Battery9V",
        "Matchbox"
    };
    
    ref array<string> Food = {
        "TunaCan",
        "SardinesCan",
        "PeachesCan",
        "SpaghettiCan",
        "BakedBeansCan",
        "TacticalBaconCan",
        "TunaCan",
        "TunaCan",
        "TunaCan",
        "TunaCan",
        "TunaCan",

    };
    ref array<string> Drink = {
        "SodaCan_Cola",
        "SodaCan_Pipsi",
        "SodaCan_Spite",
        "Canteen",

    };
    ref array<string> Medical = {
        "SalineBag",
        "BandageDressing",
        "BloodBagEmpty",
        "BloodTestKit",
        "StartKitIV",
        "TetracyclineAntibiotics",
        "Morphine",
        "BandageDressing"
    };
    ref array<string> Ammo = {
        "AmmoBox_380_35Rnd",
        "AmmoBox_9x19_25Rnd",
        "AmmoBox_762x39_20Rnd",
        "AmmoBox_762x54_20Rnd",
        "AmmoBox_45ACP_25Rnd",
    };
    ref array<string> Grenades = {
        "RDG2SmokeGrenade_White",
        "FlashGrenade",
        "RDG2SmokeGrenade_Black",
        "RDG5Grenade"
        
    };


    //all attachment classes for weapons
    ref array<string> Attachments = {
        "AK_Bayonet",
        "M9A1_Bayonet",
        "Mosin_Bayonet",
        "SKS_Bayonet",
        "Mosin_Compensator",
        "MP5_Compensator",
        "ImprovisedSuppressor",
        "M4_Suppressor",
        "AK_Suppressor",
        "PistolSuppressor",
        "MakarovPBSuppressor",
        "UniversalLight",
        "TLRLight",
        "M4_CarryHandleOptic",
        "BUISOptic",
        "M68Optic",
        "M4_T3NRDSOptic",
        "FNP45_MRDSOptic",
        "ReflexOptic",
        "ACOGOptic",
        "PUScopeOptic",
        "KashtanOptic",
        "LongrangeOptic",
        "HuntingOptic",
        "PistolOptic",
        "PSO1Optic",
        "PSO11Optic",
        "KobraOptic",
        "KazuarOptic",
        "M4_OEBttstck",
        "M4_OEBttstck_Black",
        "M4_OEBttstck_Green",
        "M4_MPBttstck",
        "M4_MPBttstck_Black",
        "M4_MPBttstck_Green",
        "M4_CQBBttstck",
        "M4_CQBBttstck_Black",
        "M4_CQBBttstck_Green",
        "AK_WoodBttstck",
        "AK_WoodBttstck_Black",
        "AK_WoodBttstck_Camo",
        "AK74_WoodBttstck",
        "AK74_WoodBttstck_Black",
        "AK74_WoodBttstck_Camo",
        "AK_FoldingBttstck",
        "AK_FoldingBttstck_Black",
        "AK_FoldingBttstck_Green",
        "AK_PlasticBttstck",
        "AK_PlasticBttstck_Black",
        "AK_PlasticBttstck_Green",
        "AKS74U_Bttstck",
        "AKS74U_Bttstck_Black",
        "AKS74U_Bttstck_Green",
        "MP5k_StockBttstck",
        "Red9Bttstck",
        "Fal_OeBttstck",
        "Fal_FoldingBttstck",
        "Saiga_Bttstck",
        "M4_PlasticHndgrd",
        "M4_PlasticHndgrd_Black",
        "M4_PlasticHndgrd_Green",
        "M4_RISHndgrd",
        "M4_RISHndgrd_Black",
        "M4_RISHndgrd_Green",
        "M4_MPHndgrd",
        "M4_MPHndgrd_Black",
        "M4_MPHndgrd_Green",
        "AK_WoodHndgrd",
        "AK_WoodHndgrd_Black",
        "AK_WoodHndgrd_Camo",
        "AK74_Hndgrd",
        "AK74_Hndgrd_Black",
        "AK74_Hndgrd_Camo",
        "AK_RailHndgrd",
        "AK_RailHndgrd_Black",
        "AK_RailHndgrd_Green",
        "AK_PlasticHndgrd",
        "MP5_PlasticHndgrd",
        "MP5_RailHndgrd",
        "M249_Hndgrd",
        "M249_RisHndgrd",
        "AtlasBipod",
        "M249_Bipod",
        "GhillieAtt_Mossy",
        "GhillieAtt_Tan",
        "GhillieAtt_Woodland"
    };


    float chance_to_spawn_building = 0.95;
    float chance_to_spawn_pile = 0.5;

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "loot_settings.json";
    }
    override void Save()
    {
        JsonFileLoader<BattleRoyaleLootData>.JsonSaveFile(GetPath(), this);
    }
    override void Load()
    {
        JsonFileLoader<BattleRoyaleLootData>.JsonLoadFile(GetPath(), this);
    }
}