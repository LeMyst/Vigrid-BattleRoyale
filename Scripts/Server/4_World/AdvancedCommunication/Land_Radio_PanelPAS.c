#ifdef SERVER
//--- Give every PAS panel on the map a charged car battery at each lobby opening.
//--- Server-only on purpose: the 2023 version also ran on clients, giving each one a local battery the
//--- server never knew about - the likely cause of the client crashes that got it disabled.
modded class Land_Radio_PanelPAS
{
    protected static ref array<Land_Radio_PanelPAS> s_BR_Panels;

    static const float BR_BATTERY_ENERGY_MIN = 0.5;
    static const float BR_BATTERY_ENERGY_MAX = 0.9;

    override void EEInit()
    {
        super.EEInit();

        //--- No spawn here: map objects run EEInit during world load, before the mission is up.
        if ( !s_BR_Panels )
            s_BR_Panels = new array<Land_Radio_PanelPAS>();

        if ( s_BR_Panels.Find( this ) == -1 )
            s_BR_Panels.Insert( this );
    }

    override void EEDelete( EntityAI parent )
    {
        if ( s_BR_Panels )
            s_BR_Panels.RemoveItem( this );

        super.EEDelete( parent );
    }

    //--- Fit a battery if the slot is empty, otherwise recharge the one already there.
    //--- Returns true when a new battery was created.
    bool BR_RefillBattery()
    {
        int slot_id = InventorySlots.GetSlotIdFromString( "LargeBattery" );
        EntityAI battery = GetInventory().FindAttachment( slot_id );
        bool created = false;

        if ( !battery )
        {
            battery = GetInventory().CreateAttachmentEx( "CarBattery", slot_id );
            created = ( battery != null );
        }

        if ( battery && battery.GetCompEM() )
            battery.GetCompEM().SetEnergy0To1( Math.RandomFloatInclusive( BR_BATTERY_ENERGY_MIN, BR_BATTERY_ENERGY_MAX ) );

        return created;
    }

    static void BR_RefillAll()
    {
        int panels = 0;
        int created = 0;
        int failed = 0;

        if ( s_BR_Panels )
        {
            for ( int i = s_BR_Panels.Count() - 1; i >= 0; i-- )
            {
                Land_Radio_PanelPAS panel = s_BR_Panels.Get( i );
                if ( !panel )
                {
                    s_BR_Panels.Remove( i );
                    continue;
                }

                panels++;
                if ( panel.BR_RefillBattery() )
                    created++;
                else if ( !panel.GetInventory().FindAttachment( InventorySlots.GetSlotIdFromString( "LargeBattery" ) ) )
                    failed++;
            }
        }

        string summary = "[PAS] " + panels + " panel(s): " + created + " battery(ies) created";
        summary = summary + ", " + ( panels - created - failed ) + " recharged, " + failed + " failed";
        BattleRoyaleUtils.Info( summary );
    }

    //--- Nearest registered panel to `pos`, or null.
    static Land_Radio_PanelPAS BR_FindNearest( vector pos )
    {
        Land_Radio_PanelPAS best = null;
        float best_dist_sq = 0;

        if ( !s_BR_Panels )
            return null;

        for ( int i = 0; i < s_BR_Panels.Count(); i++ )
        {
            Land_Radio_PanelPAS panel = s_BR_Panels.Get( i );
            if ( !panel )
                continue;

            float dist_sq = vector.DistanceSq( pos, panel.GetPosition() );
            if ( !best || dist_sq < best_dist_sq )
            {
                best = panel;
                best_dist_sq = dist_sq;
            }
        }

        return best;
    }
};
#endif
