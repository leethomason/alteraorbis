/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "itemscript.h"
#include "../tinyxml2/tinyxml2.h"

using namespace tinyxml2;
using namespace grinliz;

void ItemStorage::Load( const char* path )
{
	XMLDocument doc;
	doc.LoadFile( path );
	GLASSERT( !doc.Error() );
	if ( !doc.Error() ) {
		const XMLElement* itemsEle = doc.RootElement();
		GLASSERT( itemsEle );
		for( const XMLElement* itemEle = itemsEle->FirstChildElement( "item" );
			 itemEle;
			 itemEle = itemEle->NextSiblingElement( "item" ) )
		{
			GameItem* item = new GameItem();
			item->Load( itemEle );
			item->key = item->name;
			GLASSERT( !map.Query( item->key.c_str(), 0 ));
			map.Add( item->key.c_str(), item );

			const XMLElement* intrinsicEle = itemEle->FirstChildElement( "intrinsics" );
			if ( intrinsicEle ) {
				int nSub = 0;
				for( const XMLElement* subItemEle = intrinsicEle->FirstChildElement( "item" );
					 subItemEle;
					 subItemEle = subItemEle->NextSiblingElement( "item" ), ++nSub )
				{
					GameItem* subItem = new GameItem();
					subItem->Load( subItemEle );

					// Patch the name to make a sub-item
					GLString str;
					str.Format( "%s.%d", item->Name(), nSub );
					subItem->key = StringPool::Intern( str.c_str() );
					GLASSERT( !map.Query( subItem->key.c_str(), 0 ));
					map.Add( subItem->key.c_str(), subItem );

					item->Apply( subItem );
				}
			}
		}
	}
}


const GameItem* ItemStorage::Get( const char* name, int intrinsic )
{
	GameItem* item=0;
	if ( intrinsic >= 0 ) {
		grinliz::CStr< MAX_ITEM_NAME > n;
		n.Format( "%s.%d", name, intrinsic );
		map.Query( n.c_str(), &item );
	}
	else {
		map.Query( name, &item );
	}
	return item;
}


void ItemStorage::Get( const char* name, GameItemArr* arr )
{
	GameItem* item=0;
	map.Query( name, &item );
	GLASSERT( item );
	arr->Push( item );

	for( int i=0; true; ++i ) {
		grinliz::CStr< MAX_ITEM_NAME > n;
		n.Format( "%s.%d", name, i );
		item = 0;
		map.Query( n.c_str(), &item );

		if ( !item ) 
			break;
		arr->Push( item );
	}
}


void GunGenerator::Generate( GameItem* item, int type, int features, int effect, float level, U32 seed )
{

}


