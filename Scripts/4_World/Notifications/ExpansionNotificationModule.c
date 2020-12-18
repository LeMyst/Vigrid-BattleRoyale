modded class ExpansionNotificationModule
{
    override void AddNotification( ref NotificationRuntimeData data )
    {
        bool original = true;
        //--- this ensures the original function runs as if the user has notifications on (without perminantly changing their setting)
        if(GetExpansionClientSettings())
        {
            original = GetExpansionClientSettings().ShowNotifications;
            GetExpansionClientSettings().ShowNotifications = true;       
        }
        super.AddNotification( data );

        if(GetExpansionClientSettings())
        {
            GetExpansionClientSettings().ShowNotifications = original;
        }
    }
}