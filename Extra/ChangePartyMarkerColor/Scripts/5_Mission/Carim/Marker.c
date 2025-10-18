modded class CarimMenuMarker {
	override void CarimOnUpdate()
	{
		super.CarimOnUpdate();

		float alpha = carimNametag.GetAlpha();
		carimNametag.SetColor( ARGB( alpha * 255, 0, 255, 0 ) ); //green
	}
}