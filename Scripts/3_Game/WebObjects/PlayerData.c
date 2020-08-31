class PlayerData 
{
	//here is an example of the json data we expect to parse into this class
	//{"name":"Lystic","steam_id":"76561198277370562","match_ids":[],"rating":1000,"ips":["173.69.172.153"],"_id":"5f024bbee9e65b9c7f824cea"}
	string name;
	string steam_id;
	ref array<string> match_ids;
	int rating;
	ref array<string> ips;
	string _id;
	ref array<string> shop_purchases;
}