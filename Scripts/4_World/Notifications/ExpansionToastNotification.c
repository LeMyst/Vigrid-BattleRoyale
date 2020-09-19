modded class ExpansionNotificationUI {
    override void ShowNotification()
	{
        //this ensures we get functionality as if the setting == true, but doesn't perminantly change the users setting
        bool original = GetExpansionClientSettings().ShowNotifications;
        GetExpansionClientSettings().ShowNotifications = true;

        super.ShowNotification();

        GetExpansionClientSettings().ShowNotifications = original;
    }
}