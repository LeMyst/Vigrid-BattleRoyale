modded class Land_Radio_PanelBig
{
    override void EEInit()
    {
        super.EEInit();
        GetInventory().CreateInInventory( "CarBattery" );
    }
}
