modded class Weapon_Base {

    override void EEFired (int muzzleType, int mode, string ammoType) 
    {
        if(GetGame().IsServer())
        {
            //! server - add Fired event!
        }
        super.EEFired(muzzleType, mode, ammoType);
    }
}