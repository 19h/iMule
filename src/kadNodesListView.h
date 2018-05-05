//
// C++ Interface: kadNodesListView
//
// Description: list of kad contacts in the routing zone
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _KAD_NODES_LIST_VIEW_H_
#define _KAD_NODES_LIST_VIEW_H_

#include <wx/wx.h>
#include "GuiEvents.h"
#include <vector>
#include <map>

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

namespace Kademlia
{
class CContact ;
class CUInt128 ;
}

class KadNodesListView : public CMuleListCtrl
{
public:
        KadNodesListView(wxWindow* parent, wxWindowID id=-1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxLC_ICON, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxT("kadNodesListView"));

        void AddNode       ( Kademlia::CContact contact  );
        void DeleteNode    ( Kademlia::CContact contact  );
        void OnColumnLClick( wxListEvent & event );

protected:
        /// Return old column order.
        wxString GetOldColumnOrder() const;

        /**
         * Sorter function used by wxListCtrl::SortItems function.
         *
         * @see CMuleListCtrl::SetSortFunc
         * @see wxListCtrl::SortItems
         */
        static int wxCALLBACK SortProc(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData);


private :
	void SortItems();
	void UpdateItemColor(long index);

        static std::map<wxUIntPtr, Kademlia::CContact> _contacts_map;

        DECLARE_EVENT_TABLE()
};
#endif
