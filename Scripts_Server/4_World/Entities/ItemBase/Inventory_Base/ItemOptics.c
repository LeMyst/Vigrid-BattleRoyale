#ifdef SERVER
// Combat Sights
modded class M68Optic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// RVN Sights
modded class M4_T3NRDSOptic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// Mini Sights
modded class FNP45_MRDSOptic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// Baraka Sights
modded class ReflexOptic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// PSO-1 Scope
modded class PSO1Optic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// PSO-1-1 Scope
modded class PSO11Optic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// Kobra Sights
modded class KobraOptic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// 1PN51 Scope
modded class KazuarOptic
{
    override void EEInit()
    {
        super.EEInit();

        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}

// NV-PVS4 Scope
modded class M4_T3NRDSOptic
{
    override void EEInit()
    {
        super.EEInit();
        
        if(HasEnergyManager())
        {
            GetInventory().CreateInInventory( "Battery9V" );
        }
    }
}
#endif