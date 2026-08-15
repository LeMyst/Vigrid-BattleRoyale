#ifdef SERVER
modded class CarScript
{
    override void EEOnCECreate()
    {
        //  super is NOT optional here. Vanilla's own body fills 0-35% fuel, and DayZ-Expansion
        //  overrides this method to run m_ExpansionVehicle.OnCECreate() - dropping super dropped
        //  Expansion's entire CE-spawn init for every car the Central Economy creates.
        super.EEOnCECreate();

        //  Fill() ADDS to the tank and clamps at capacity (see the proto doc in P:\scripts\3_game\
        //  vehicles\car.c), so vanilla's 0-35% would stack under the refill below and skew it
        //  upward. Drain first and the range stays exactly what the line after it says.
        LeakAll( CarFluid.FUEL );

        //  Random 30-100% fuel, full coolant and oil.
        Fill( CarFluid.FUEL, GetFluidCapacity( CarFluid.FUEL ) * Math.RandomFloatInclusive(0.30, 1) );
        Fill( CarFluid.COOLANT, GetFluidCapacity( CarFluid.COOLANT ) );
        Fill( CarFluid.OIL, GetFluidCapacity( CarFluid.OIL ) );
    }
};
#endif
