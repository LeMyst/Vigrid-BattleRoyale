#ifdef SERVER
modded class BoatScript
{
    override void EEOnCECreate()
    {
        //  Same reasoning as CarScript in this addon: vanilla fills 0-35% and DayZ-Expansion
        //  overrides this to run m_ExpansionVehicle.OnCECreate(), so super must be called, and
        //  Fill() adds rather than sets - drain first so the refill below is exactly 30-100%.
        super.EEOnCECreate();

        LeakAll( BoatFluid.FUEL );

        //  Random 30-100% fuel.
        Fill( BoatFluid.FUEL, GetFluidCapacity( BoatFluid.FUEL ) * Math.RandomFloatInclusive(0.30, 1) );
    }
};
#endif
