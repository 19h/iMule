//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2009 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef KADDLG_H
#define KADDLG_H

#include <wx/panel.h>		// Needed for wxPanel
#include <wx/sizer.h>
#include "muuli_wdr.h"
#include "kadNodesListView.h"

namespace Kademlia { class CContact; }
class COScopeCtrl;
class wxListEvent;
class wxCommandEvent;
class wxMouseEvent;
class CMuleTextCtrl;
typedef struct UpdateInfo GraphUpdateInfo;


class CKadDlg : public wxPanel
{
public:
        CKadDlg(wxWindow* pParent);
        ~CKadDlg() {};

        void Init();
        void InitSplit();
        void SetUpdatePeriod(int step);
        void SetGraphColors();
        void UpdateGraph(const GraphUpdateInfo& update);

        void AddNode(Kademlia::CContact node);
        void UpdateNode(Kademlia::CContact node);
        void DeleteNode(Kademlia::CContact node);

        KadNodesListView * GetKadNodesListView() { return m_nodes_list; }
        wxStaticBoxSizer * GetKadNodesBox() { return m_nodes_box; }
private:
        COScopeCtrl* m_kad_scope;
        KadNodesListView * m_nodes_list;
        wxStaticBoxSizer * m_nodes_box;

        uint16 m_nbNodes;
        
        // Event handlers
        void        OnBnClickedCopyMyDestToClipboard(wxCommandEvent& evt);
        void		OnBnClickedBootstrapClient(wxCommandEvent& evt);
        void		OnBnClickedBootstrapKnown(wxCommandEvent& evt);
        void		OnBnClickedDisconnectKad(wxCommandEvent& evt);
        void		OnBnClickedUpdateNodeList(wxCommandEvent& evt);
        void		OnFieldsChange(wxCommandEvent& evt);
        void        OnBnClickedNodeExport(wxCommandEvent& evt);

        // WDR: method declarations for CKadDlg
        CMuleTextCtrl* GetNodeDestCtrl()  { return (CMuleTextCtrl*) FindWindow( ID_NODE_DEST ); }
        CMuleTextCtrl* GetNodesListUrlCtrl()  { return (CMuleTextCtrl*) FindWindow( IDC_NODESLISTURL ); }
        KadNodesListView* GetListctrl()  { return (KadNodesListView*) FindWindow( ID_KadDlgNodesList ); }
        COScopeCtrl* GetKadscope();

        DECLARE_EVENT_TABLE()
};

#endif // KADDLG_H
// File_checked_for_headers