#ifdef SERVER
modded class ItemBase
{
    override void EEInit()
    {
        super.EEInit();

        ExpansionDeferredCreateCleanup();
    }
};
#endif