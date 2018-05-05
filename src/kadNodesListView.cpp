#include "kadNodesListView.h"
#include "kademlia/routing/Contact.h"

#include "common/Format.h"
#include "Logger.h"

#include "MuleThread.h"

#include "MuleColour.h"
#include <wx/wupdlock.h>

enum columns_enum {
	TYPE = 0, 
	CONTACT_KAD_VERSION, 
	UDP_READABLE, 
	TCP_READABLE, 
	LAST_SEEN, 
	KAD_ID_STRING, 
	KAD_ID, 
	DISTANCE, 
	UDP_ID
};

std::map<wxUIntPtr, Kademlia::CContact> KadNodesListView::_contacts_map;

// static columns_enum __sort_order = TYPE ;
// static bool __reverse = false ;
//static wiMutex contacts_mutex ;



KadNodesListView::KadNodesListView ( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name )
        :
        CMuleListCtrl(parent, id, pos, size, style | wxLC_REPORT, validator, name)
{
        // Setting the sorter function.
        SetSortFunc( SortProc );

        InsertColumn( TYPE,    _("node type"), wxLIST_FORMAT_CENTRE, 50, wxT("n") );
        InsertColumn( CONTACT_KAD_VERSION,    _("version"), wxLIST_FORMAT_CENTRE, 50, wxT("v") );
        InsertColumn( UDP_READABLE,    _("udp"), wxLIST_FORMAT_CENTRE, 100, wxT("u") );
        InsertColumn( TCP_READABLE,    _("tcp"), wxLIST_FORMAT_CENTRE, 100, wxT("t") );
        InsertColumn( LAST_SEEN,    _("last time seen"), wxLIST_FORMAT_CENTRE, 100, wxT("l") );
        InsertColumn( KAD_ID_STRING,    _("kad str"), wxLIST_FORMAT_CENTRE, 100, wxT("s") );
        InsertColumn( KAD_ID,    _("kad id"), wxLIST_FORMAT_CENTRE, 50, wxT("k") );
        InsertColumn( DISTANCE,    _("distance"), wxLIST_FORMAT_CENTRE, 50, wxT("d") );
        InsertColumn( UDP_ID,    _("udp key"), wxLIST_FORMAT_CENTRE, 50, wxT("U") );

	SetTableName( wxT("KadNodes") );
	LoadSettings();
}

wxString KadNodesListView::GetOldColumnOrder() const
{
    return wxT("n,v,u,t,l,s,k,d,U");
}

void KadNodesListView::AddNode( Kademlia::CContact contact )
{
        /* Freeze all refresh during window display */
        wxWindowUpdateLocker locker(this);
        
        Kademlia::CContact node = contact ;
        AddDebugLogLineN(logKadPrefs, CFormat ( wxT ( "KadNodesListView::AddNode" ) ) );

	const wxUIntPtr toshowdata = (wxUIntPtr)(node.GetClientID().Get32BitChunk(0));

        // is this node being added or updated ?
        bool updated = _contacts_map.count ( toshowdata )>0 ;
	long id;
        // 	bool resort = false ;
        if (!updated) {
		_contacts_map[ toshowdata ] = node;
		long insertPos = GetInsertPos(toshowdata);
		id = InsertItem(insertPos, wxString() << node.GetType());
		SetItemPtrData(id, toshowdata);
		AddDebugLogLineN(logKadPrefs,
                                 CFormat ( wxT ( "Adding unknown  contact n°%ld" ) ) % id );
        } else {
		id = FindItem(-1, (wxUIntPtr)(toshowdata));
		AddDebugLogLineN(logKadPrefs,
				CFormat ( wxT ( "Updating contact %d" ) ) % node.GetClientID().Get32BitChunk(0) );
		_contacts_map[ toshowdata ] = node;
        }

	SetItem(id, TYPE, wxString::Format("%" PRIu8 "", node.GetType()));
	SetItem(id, CONTACT_KAD_VERSION, wxString::Format("%" PRIu8 "", node.GetVersion()));
	SetItem(id, UDP_READABLE, node.GetUDPDest().humanReadable());
	SetItem(id, TCP_READABLE,  node.GetTCPDest().humanReadable());
	time_t lastSeen = node.GetLastSeen() ;
	SetItem(id, LAST_SEEN, lastSeen==0 ? wxString()<<_( "never" ) : wxDateTime(lastSeen).Format(wxT("%c")));
	SetItem(id, KAD_ID_STRING, node.GetClientIDString());
	SetItem(id, KAD_ID, node.GetClientID().ToBinaryString().Mid(0,12));
	SetItem(id, DISTANCE, node.GetDistance().ToBinaryString().Mid(0,12));
	SetItem(id, UDP_ID, node.GetUDPDest().toString());

        UpdateItemColor( id );
	// Deletions of items causes rather large amount of flicker, so to
	// avoid this, we resort the list to ensure correct ordering.
	if (!IsItemSorted(id)) {
		SortList();
	}
}

void KadNodesListView::DeleteNode( Kademlia::CContact contact )
{
        /* Freeze all refresh during window display */
        wxWindowUpdateLocker locker(this);

        Kademlia::CContact node = contact ;

	const wxUIntPtr toremove = (wxUIntPtr)(node.GetClientID().Get32BitChunk(0));
        long index = FindItem(-1, toremove);
	if (index != -1) {
		AddDebugLogLineN(logKadPrefs,
                                 CFormat ( wxT ( "Removing contact n°%ld" ) ) % index );
                DeleteItem(index);
	}

        wxASSERT(_contacts_map.count(toremove));
        _contacts_map.erase(toremove);
	
}

void KadNodesListView::UpdateItemColor(long int index)
{
        wxListItem item;
        item.SetId( index );
        item.SetColumn( TYPE );
        item.SetMask(
                wxLIST_MASK_STATE |
                wxLIST_MASK_TEXT |
                wxLIST_MASK_IMAGE |
                wxLIST_MASK_DATA |
                wxLIST_MASK_WIDTH |
                wxLIST_MASK_FORMAT);

        if ( GetItem(item) ) {
                CMuleColour newcol(wxSYS_COLOUR_WINDOWTEXT);

                Kademlia::CContact node = _contacts_map[GetItemData(index)];

                int red		= newcol.Red();
                int green		= newcol.Green();
                int blue		= newcol.Blue();

                switch (node.GetType()) {
                case 0 :
                        // File has already been downloaded. Mark as green.
			green = 255;
			red = 0;
			break;
		case 1 :
			green = 200 ;
			red	= 100;
			break;
		case 2 :
			green = 100 ;
			red = 200 ;
			break;
		case 3 :
			green = 0 ;
			red = 255;
			break;
		case 4 :
			green = 0 ;
			red = 0 ;
			break;
		default:
			break;
		}
		switch (node.GetVersion()) {
			case 0 :
				blue = 0 ;
				break;
			case 8 :
				blue = 255 ;
				break;
			default:
				break;
		}
		// don't forget to set the item data back...
		wxListItem newitem;
		newitem.SetId( index );
		newitem.SetTextColour( wxColour( red, green, blue ) );
		SetItem( newitem );
	}
}

int KadNodesListView::SortProc(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData)
{
	int modifier = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;

	Kademlia::CContact node1 = _contacts_map[item1] ;
	Kademlia::CContact node2 = _contacts_map[item2] ;
	
	int result = 0;
	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
	// Sort by filename
	case TYPE :			result = CmpAny(node1.GetType(), node2.GetType());
		break;
        case CONTACT_KAD_VERSION :  	result = CmpAny(node1.GetVersion(), node2.GetVersion());
		break;
        case UDP_READABLE :  		result = CmpAny(node1.GetUDPDest().humanReadable(), node2.GetUDPDest().humanReadable()) ;
		break;
        case TCP_READABLE :  		result = CmpAny(node1.GetTCPDest().humanReadable(),node2.GetTCPDest().humanReadable()) ;
		break;
        case LAST_SEEN    :  		result = CmpAny(node1.GetLastSeen(), node2.GetLastSeen());
		break;
	case KAD_ID_STRING:  		result = CmpAny(node1.GetClientIDString(),node2.GetClientIDString()) ;
		break;
        case KAD_ID       :  		result = CmpAny(node1.GetClientID().ToBinaryString().Mid ( 0,12 ),node2.GetClientID().ToBinaryString().Mid ( 0,12 )) ;
		break;
        case DISTANCE     :  		result = CmpAny(node1.GetDistance().ToBinaryString().Mid ( 0,12 ),node2.GetDistance().ToBinaryString().Mid ( 0,12 )) ;
		break;
        case UDP_ID       :  		result = CmpAny(node1.GetUDPDest().toString(),node2.GetUDPDest().toString()) ;
		break;
	}
	return modifier*result;
}



void KadNodesListView::OnColumnLClick ( wxListEvent & event )
{
        AddDebugLogLineN(logKadPrefs,
                         CFormat ( wxT ( "ColumnLClick enter °%d" ) ) % (int) event.GetColumn() );
        wxASSERT ( event.GetColumn() >= 0 && event.GetColumn() <= UDP_ID );
        // Let the real event handler do its work first
        CMuleListCtrl::OnColumnLClick( event );
}


BEGIN_EVENT_TABLE ( KadNodesListView, CMuleListCtrl )
        EVT_LIST_COL_CLICK ( -1, KadNodesListView::OnColumnLClick )
END_EVENT_TABLE()
