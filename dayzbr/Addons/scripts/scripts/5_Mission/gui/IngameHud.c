modded class IngameHud
{
	ref TextWidget m_BrNotificationWidget;
	ref WidgetFadeTimer fade_timer_notification;
	override void Init( Widget hud_panel_widget )
	{
		super.Init(hud_panel_widget);
		m_BrNotificationWidget = m_HudPanelWidget.FindAnyWidget("BrNotificationWidget");
	}
	
	void DisplayNotification(string notification)
	{
		m_BrNotificationWidget.SetText(notification);
		//instant fade in
		
		if(fade_timer_notification)
		{
			m_FadeTimers.RemoveItem(fade_timer_notification);
		}
		
		m_BrNotificationWidget.SetAlpha(1);
		m_BrNotificationWidget.Show(true);
		
		//fade out over 10 seconds
		fade_timer_notification = new WidgetFadeTimer;
		fade_timer_notification.FadeOut(m_BrNotificationWidget, 10, false);
		m_FadeTimers.Insert(fade_timer_notification);
	}
}
