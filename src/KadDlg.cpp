//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2011 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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

#include "KadDlg.h"
#include "muuli_wdr.h"
#include "kadNodesListView.h"
#include "common/Format.h"
#include "OScopeCtrl.h"
#include "OtherFunctions.h"
#include "HTTPDownload.h"
#include "Logger.h"
#include "amule.h"
#include "Preferences.h"
#include "StatisticsDlg.h"
#include "ColorFrameCtrl.h"
#include "MuleTextCtrl.h" // Needed for CMuleTextCtrl

#include "MuleColour.h"
#include "Statistics.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/routing/RoutingZone.h"
#include "kademlia/routing/Contact.h"

#ifndef CLIENT_GUI
#include "kademlia/kademlia/Kademlia.h"
#endif


BEGIN_EVENT_TABLE(CKadDlg, wxPanel)
        EVT_TEXT(ID_NODE_DEST, CKadDlg::OnFieldsChange)

        EVT_TEXT_ENTER(IDC_NODESLISTURL ,CKadDlg::OnBnClickedUpdateNodeList)

        EVT_BUTTON(ID_MYDESTTOCLIPBOARD, CKadDlg::OnBnClickedCopyMyDestToClipboard)
        EVT_BUTTON(ID_NODECONNECT, CKadDlg::OnBnClickedBootstrapClient)
//         EVT_BUTTON(ID_KNOWNNODECONNECT, CKadDlg::OnBnClickedBootstrapKnown)
        EVT_BUTTON(ID_UPDATEKADLIST, CKadDlg::OnBnClickedUpdateNodeList)
        EVT_BUTTON(ID_NODEEXPORT, CKadDlg::OnBnClickedNodeExport)
END_EVENT_TABLE();



CKadDlg::CKadDlg(wxWindow* pParent)
        : wxPanel(pParent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxT("kadwnd") )
{
        m_kad_scope = NULL;
        m_nodes_list = NULL;
}





COScopeCtrl* CKadDlg::GetKadscope()
{
        return CastChild(wxT("kadScope"), COScopeCtrl);
}


void CKadDlg::Init()
{
        m_nodes_box  = ( wxStaticBoxSizer * ) kadDlgNodesGraphFrame ;
        m_nodes_list = GetListctrl();
        m_kad_scope  = GetKadscope();
        m_kad_scope->SetRanges(0.0, thePrefs::GetStatsMax());
        m_kad_scope->SetYUnits(wxT("Nodes"));
        m_nbNodes = 0;

#ifndef __WXMSW__
        //
        // Get label with line breaks out of muuli.wdr, because generated code fails
        // to compile in Windows.
        //
        // In Windows, setting a button label with a newline fails (the newline is ignored).
        // Creating a button with such a label works however. :-/
        // So leave the label from the muuli (without line breaks) here,
        // so it can still be fixed in the translation.
        //
//         wxButton* bootstrap = CastChild(ID_KNOWNNODECONNECT, wxButton);
//         bootstrap->SetLabel(_("Bootstrap from \nknown clients"));
#endif
        SetUpdatePeriod(thePrefs::GetTrafficOMeterInterval());
        SetGraphColors();
}

void CKadDlg::InitSplit()
{
        if (GetSize().GetWidth()>2) {
                CastChild(ID_SPLITTER, wxSplitterWindow)->SetSashPosition(GetSize().GetWidth()*2/3);
                CastChild(ID_SPLITTER, wxSplitterWindow)->SetSashGravity(0.6667);
        }
}

void CKadDlg::SetUpdatePeriod(int step)
{
        // this gets called after the value in Preferences/Statistics/Update delay has been changed
        if (step == 0) {
                m_kad_scope->Stop();
        } else {
                m_kad_scope->Reset(step);
        }
}


void CKadDlg::SetGraphColors()
{
        static const char aTrend[] = { 2,      1,        0        };
        static const int aRes[]    = { IDC_C0, IDC_C0_3, IDC_C0_2 };

        m_kad_scope->SetBackgroundColor(CStatisticsDlg::getColors(0));
        m_kad_scope->SetGridColor(CStatisticsDlg::getColors(1));

        for (size_t i = 0; i < 3; ++i) {
                m_kad_scope->SetPlotColor(CStatisticsDlg::getColors(12 + i), aTrend[i]);

                CColorFrameCtrl* ctrl = CastChild(aRes[i], CColorFrameCtrl);
                ctrl->SetBackgroundBrushColour(CMuleColour(CStatisticsDlg::getColors(12 + i)));
                ctrl->SetFrameBrushColour(*wxBLACK);
        }
}


void CKadDlg::UpdateGraph(const GraphUpdateInfo& update)
{
        std::vector<float *> v(3);
        v[0] = const_cast<float *>(&update.kadnodes[0]);
        v[1] = const_cast<float *>(&update.kadnodes[1]);
        v[2] = const_cast<float *>(&update.kadnodes[2]);
        const std::vector<float *> &apfKad(v);
        unsigned nodeCount = static_cast<unsigned>(update.kadnodes[2]);

        if (!IsShownOnScreen()) {
                m_kad_scope->DelayPoints();
        } else {
                // Check the current node-count to see if we should increase the graph height
                if (m_kad_scope->GetUpperLimit() < update.kadnodes[2]) {
                        // Grow the limit by 50 sized increments.
                        m_kad_scope->SetRanges(0.0, ((nodeCount + 49) / 50) * 50);
                }

                m_kad_scope->AppendPoints(update.timestamp, apfKad);
        }
        /* the number of nodes is handled by AddNode and RemoveNode in iMule
        wxStaticText* label = CastChild( wxT("nodesListLabel"), wxStaticText );
        wxCHECK_RET(label, wxT("Failed to find kad-nodes label"));

        label->SetLabel(CFormat(_("Nodes (%u)")) % nodeCount);
        label->GetParent()->Layout();
        */
}


// Enables or disables the node connect button depending on the conents of the text fields
void CKadDlg::OnFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
        // Enable the node connect button if all fields contain text
        wxString txtaddr = this->GetNodeDestCtrl()->GetValue() ;
        CI2PAddress dest = CI2PAddress::fromString(txtaddr);
        FindWindowById(ID_NODECONNECT)->Enable( dest.isValid() );
}

void CKadDlg::OnBnClickedCopyMyDestToClipboard(wxCommandEvent& WXUNUSED(evt))
{
#ifndef CLIENT_GUI
        if (FindWindowById(ID_MYDESTTOCLIPBOARD)->IsEnabled())
#endif
        {
                theApp->CopyTextToClipboard(theApp->GetUdpDest().toString());
        }
}


void CKadDlg::OnBnClickedBootstrapClient(wxCommandEvent& WXUNUSED(evt))
{
        if (FindWindowById(ID_NODECONNECT)->IsEnabled()) {

                wxString txtaddr = ((wxTextCtrl*)FindWindowById( ID_NODE_DEST ))->GetValue() ;
                CI2PAddress dest = CI2PAddress::fromString(txtaddr);

                AddDebugLogLineN(logKadPrefs, CFormat(wxT("bootstrap on %s ")) % dest.humanReadable());

                if (dest.isValid()) {
                        theApp->BootstrapKad(dest);
                } else {
                        wxMessageBox(_("Invalid ip to bootstrap"), _("Warning"), wxOK | wxICON_EXCLAMATION, this);
                }
        } else {
                wxMessageBox(_("Please fill all fields required"), _("Message"), wxOK | wxICON_INFORMATION, this);
        }
}

void CKadDlg::OnBnClickedNodeExport(wxCommandEvent& WXUNUSED(evt))
{
        if ( not theApp->IsKadRunning() ) {
                wxMessageDialog md(NULL,
                                   _("Kademlia is not running.\n No known node !"),
                                   _("Export nodes destinations"));
                md.ShowModal();
                return ;
        }
        wxString name = wxFileSelector(_("Select file to export to"));
        if (!name.empty()) {
                theApp->exportKadContactsOnFile(name);
        }
}

void CKadDlg::OnBnClickedBootstrapKnown(wxCommandEvent& WXUNUSED(evt))
{
        theApp->StartKad();
}


void CKadDlg::OnBnClickedDisconnectKad(wxCommandEvent& WXUNUSED(evt))
{
        theApp->StopKad();
}


void CKadDlg::OnBnClickedUpdateNodeList(wxCommandEvent& WXUNUSED(evt))
{
        if ( wxMessageBox( wxString(_("Are you sure you want to download a new nodes.dat file?\n")) +
                           _("Doing so will remove your current nodes and restart Kademlia connection.")
                           , _("Continue?"), wxICON_EXCLAMATION | wxYES_NO, this) == wxYES ) {
                wxString strURL = this->GetNodesListUrlCtrl()->GetValue();

                thePrefs::SetKadNodesUrl(strURL);
                theApp->UpdateNotesDat(strURL);
        }
}

void CKadDlg::AddNode(Kademlia::CContact node)
{
        ++m_nbNodes;
        GetKadNodesListView()->AddNode(node);
        GetKadNodesBox()->GetStaticBox()->SetLabel(wxString::Format(_("Nodes stats (%" PRIu16 ")"), m_nbNodes));
}

void CKadDlg::UpdateNode(Kademlia::CContact node)
{
        GetKadNodesListView()->AddNode(node);
}

void CKadDlg::DeleteNode(Kademlia::CContact node)
{
        --m_nbNodes;
        GetKadNodesListView()->DeleteNode(node);
        GetKadNodesBox()->GetStaticBox()->SetLabel(wxString::Format(_("Nodes stats (%" PRIu16 ")"), m_nbNodes));
}

// File_checked_for_headers