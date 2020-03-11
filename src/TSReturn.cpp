#include "TSReturn.hpp"

// TSReturn::TSReturn ()
//     : m_tsBase (0)
// {
// }

// TSReturn::~TSReturn ()
// {
// }

// void TSReturn::reset (void)
// {
//     m_tsBase = 0;
//     m_vec.clear ();
// }

// void TSReturn::addEntry (uint32_t seq, uint64_t tsRecv)
// {
//     if (m_tsBase == 0)
//     {
// 	// first entry
// 	m_tsBase = tsRecv;
//     }
//     uint32_t tsDiff = (uint32_t)(tsRecv - m_tsBase);
//     m_vec.emplace_back (seq, tsDiff);
// }

// format is magic packet 'T' 'I' 'M' 'E', followed by number of entries, then
// timestamp base, then entries, 
// std::string TSReturn::toString (void)
// {
//     uint32_t nEnt = m_vec.size ();
//     // uint32_t magic = *((uint32_t *)"TIME");
//     return std::string ("EMIT") + std::string ((const char *)&nEnt, 4) + std::string ((const char *)&m_tsBase, 8) + std::string ((const char *)m_vec.data (), 8*nEnt);
// }

// size_t TSReturn::size (void)
// {
//     return m_vec.size ();
// }

// std::pair<uint32_t,uint64_t> TSReturn::getEntry (size_t idx)
// {
//     if (idx >= this->size ())
//     {
// 	return std::make_pair (0,0);
//     }

//     const auto& ent = m_vec[idx];
//     return std::make_pair (std::get<0>(ent), m_tsBase + std::get<1>(ent));
// }

std::string TSReturn::ToString (const std::vector<std::pair<uint32_t,uint64_t> >& vec)
{
    size_t plSize = (4 // MAGIC
		     + 4 // nEnt
		     + 8 // tsBase
		     + (4 * vec.size ()) // seq
		     + (4 * vec.size ())); // tsDiff
    std::string str (plSize, 0);

    char *p = &str[0];

    // set magic
    *((uint32_t *)p) = *((uint32_t *) "TIME");

    *((uint32_t *)(p + 4)) = (uint32_t)vec.size ();

    if (vec.size () > 0)
    {
	uint64_t tsBase = vec[0].second;
	*((uint64_t *)(p + 4 + 4)) = tsBase;

	uint32_t *seqVec = (uint32_t *)(p + 4 + 4 + 8);
	uint32_t *tsVec = seqVec + vec.size ();
	for (size_t ii=0; ii < vec.size (); ++ii)
	{
	    seqVec[ii] = vec[ii].first;
	    tsVec[ii] = vec[ii].second - tsBase;
	}
    }
    return str;
}

std::vector<std::pair<uint32_t,uint64_t> > TSReturn::FromString (const std::string& str)
{
    std::vector<std::pair<uint32_t,uint64_t> > vec;
    if (!TSReturn::Valid (str))
    {
	return vec;
    }

    const char *p = str.c_str ();
    uint32_t nEnt = *((uint32_t *)(p + 4));
    uint64_t tsBase = *((uint64_t *)(p + 4 + 4));

    uint32_t *seqVec = (uint32_t *)(p + 4 + 4 + 8);
    uint32_t *tsVec = seqVec + nEnt;
    vec.resize (nEnt);
    for (size_t ii=0; ii < nEnt; ++ii)
    {
	vec[ii].first = seqVec[ii];
	vec[ii].second = tsBase + tsVec[ii];
    }
    
    return vec;
}

bool TSReturn::Valid (const std::string& str)
{
    // TODO:  also check that the length is valid as per number of entries.
    return ((str.size () > 16)
	    && (*((uint32_t *)str.c_str ()) == *((uint32_t *) "TIME")));
}
