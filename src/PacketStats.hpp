#pragma once

using SeqNum = uint32_t;
using Timestamp = uint64_t;

// maybe LinkMetadata
struct PacketStats
{
    int numBytes;
    // possibly rename this to droppedPackets
    std::vector<SeqNum> dropped;

    // is this necessary?
    std::vector<Timestamp> tsRecv;

    std::vector<Timestamp> tsDiff;
    
};
