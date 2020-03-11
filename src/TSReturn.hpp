#pragma once

#include <vector>
#include <tuple>

namespace TSReturn
{
    std::string ToString (const std::vector<std::pair<uint32_t,uint64_t> >& vec);
    std::vector<std::pair<uint32_t,uint64_t> > FromString (const std::string& str);
    bool Valid (const std::string& str);
//     TSReturn ();
//     ~TSReturn ();

//     void reset (void);
//     void addEntry (uint32_t seq, uint64_t tsRecv);

//     std::string toString (void) const;
//     size_t size (void) const;
//     std::pair<uint32_t,uint64_t> getEntry (size_t idx) const;
    
// private:
//     uint64_t m_tsBase;
//     std::vector<std::pair<uint32_t, uint32_t> > m_vec;
};
