#include "Maps.h"

#include "kademlia/utils/UInt128.h"
#include "kademlia/routing/Contact.h"
#include "kademlia/kademlia/Search.h"

using namespace std;

/**
 * ContactMap
 */

void Kademlia::ContactMap::clear()
{
        m_map.clear();
}

size_t Kademlia::ContactMap::size() const
{
        return m_map.size();
}

void Kademlia::ContactMap::apply ( std::function< void(CSearchContact&) > f )
{
        for (pair<const CUInt128, CSearchContact> & p: m_map)
        {
                f(p.second);
        }
}

void Kademlia::ContactMap::add ( Kademlia::CSearchContact contact )
{
        m_map[contact.GetClientID()] = contact;
}

std::map< CUInt128, Kademlia::CSearchContact >::iterator Kademlia::ContactMap::get ( Kademlia::CSearchContact& c )
{
        return m_map.find(c.GetClientID());
}

void Kademlia::ContactMap::moveContactTo ( Kademlia::CSearchContact contact, Kademlia::ContactMap* _map)
{
        std::map<CUInt128, CSearchContact>::iterator it = this->get(contact);
        _map->add(it->second);
        m_map.erase(it);
}

bool Kademlia::ContactMap::contains ( const Kademlia::CSearchContact& c ) const
{
        return m_map.find(c.GetClientID()) != m_map.end();
}

Kademlia::CSearchContact Kademlia::ContactMap::get_if(std::function<bool(const Kademlia::CSearchContact&)> f) const
{
        for (const std::pair<const CUInt128, CSearchContact>& p: m_map)
        {
                if (f(p.second)) return p.second;
        }
        return CSearchContact();
}

Kademlia::CSearchContact& Kademlia::ContactMap::ref ( Kademlia::CSearchContact c )
{
        return get(c)->second;
}

/**
 * TargetContactMap
 */

Kademlia::TargetContactMap::TargetContactMap ( CUInt128 target )
{
        m_target.reset(new CUInt128(target));
}

void Kademlia::TargetContactMap::add ( Kademlia::CSearchContact contact )
{
        m_map[*m_target ^ contact.GetClientID()] = contact;
}

std::map< CUInt128, Kademlia::CSearchContact >::iterator Kademlia::TargetContactMap::get ( Kademlia::CSearchContact& c )
{
        return m_map.find(*m_target ^ c.GetClientID());
}

const CUInt128 & Kademlia::TargetContactMap::getClosestDistance() const
{
        return m_map.begin()->first;
}

Kademlia::CSearchContact Kademlia::TargetContactMap::popClosest()
{
        CSearchContact c = m_map.begin()->second;
        m_map.erase(m_map.begin());
        return c;
}

void Kademlia::TargetContactMap::pop_back()
{
        map<CUInt128,CSearchContact>::iterator last = m_map.end();
        last--;
        m_map.erase(last);
}

const CUInt128& Kademlia::TargetContactMap::getFurthestDistance() const
{
        return m_map.rbegin()->first;
}

void Kademlia::TargetContactMap::moveFurthestTo( Kademlia::ContactMap * other)
{
        map<CUInt128,CSearchContact>::iterator last = m_map.end();
        last--;
        other->add(last->second);
        m_map.erase(last);
}

bool Kademlia::TargetContactMap::contains ( const Kademlia::CSearchContact& c ) const
{
        return m_map.find(*m_target ^ c.GetClientID()) != m_map.end();
}

