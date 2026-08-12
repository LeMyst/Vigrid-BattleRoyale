#ifdef SERVER
/**
 *  Pressing the detonator makes the charge yours.
 *
 *  Arming already records an activator (ExplosivesBase.OnPlacementComplete), but for a remotely
 *  detonated device the person who pressed the trigger is the one who chose the moment and the
 *  victim - and they are not necessarily the person who placed it, since the trigger is a lootable
 *  item that can change hands.
 */
modded class ActionTriggerRemotely
{
    override void OnExecuteServer(ActionData action_data)
    {
        RemoteDetonatorTrigger trigger = NULL;
        ExplosivesBase device = NULL;

        //--- BEFORE super, which is what detonates: super reaches OnActivatedByItem, and the victim's
        //--- own EEKilled reads the activator back off the device in the same call stack.
        trigger = RemoteDetonatorTrigger.Cast(action_data.m_MainItem);
        if (trigger && trigger.IsConnected())
        {
            device = ExplosivesBase.Cast(trigger.GetControlledDevice());
            if (device)
                device.BR_SetActivator(action_data.m_Player);
        }

        super.OnExecuteServer(action_data);
    }
}
#endif
