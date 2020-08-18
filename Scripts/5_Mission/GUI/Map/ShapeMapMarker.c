class ShapeMapMarker extends ExpansionMapWidgetBase
{
    protected override void OnInit( Widget layoutRoot )
	{
        //TODO: initialize widgets on layoutRoot
    }
    override string GetLayoutPath()
	{
        //-- TODO: return custom marker layout
		return "DayZExpansion/GUI/layouts/map/expansion_map_marker.layout";
	}
    override void Update( float pDt )
	{
        super.Update(pDt); //not really needed as it's just for draggable widgets (which these are not)

        //-- TODO: render this bad boy's canvas
    }

    //--- todo: rendering controls
    void SetSize(float A, float B)
    {

    }
    void SetThickness(float T)
    {

    }
    void SetColor(int color)
    {

    }
}