//
// This file is part of the imule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#ifndef __KAD_MAPS_H__
#define __KAD_MAPS_H__

#include <map>
#include <list>
#include <set>
#include <ctime>
#include <functional>
#include <memory>

////////////////////////////////////////
namespace Kademlia
{
        ////////////////////////////////////////

        class CUInt128;
        class CContact;
        class CSearchContact;
        
        typedef std::set <CContact>           ContactSet;
        typedef std::list<CContact>           ContactList;
        typedef std::list<CUInt128>           UIntList;
        
        class ContactMap
        {
        public:
                void clear() ;
                bool empty() const { return size()==0; }
                size_t size() const;
                void apply(std::function<void(CSearchContact&)>);
                virtual void add(CSearchContact contact);
                void moveContactTo(CSearchContact contact, ContactMap *);
                virtual bool contains(const CSearchContact & c) const;
                CSearchContact get_if(std::function<bool(const CSearchContact&)>) const;
                CSearchContact & ref(CSearchContact);
        protected:
                virtual std::map<CUInt128, CSearchContact>::iterator get(CSearchContact&c);
                std::map <CUInt128, CSearchContact> m_map;
        };

        class TargetContactMap : public ContactMap
        {
        public:
                TargetContactMap(CUInt128 target);
                virtual void add(CSearchContact contact);
                const CUInt128 & getClosestDistance() const;
                CSearchContact popClosest();
                void pop_back();
                const CUInt128 & getFurthestDistance() const;
                void moveFurthestTo(ContactMap *);
                virtual bool contains(const CSearchContact & c) const;
        protected:
                virtual std::map<CUInt128, CSearchContact>::iterator get(CSearchContact&c);
        private:
                std::unique_ptr<CUInt128> m_target;
        };
} // End namespace

#endif // __KAD_MAPS_H__
// File_checked_for_headers