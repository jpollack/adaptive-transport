#pragma once

class BWLimit
{
public:
    BWLimit ();
    ~BWLimit ();

    void setBandwidth (uint32_t other);
    uint64_t tsNext (uint32_t size);
    void setTsLast (uint64_t other);
private:
    uint32_t bandwidth; // bytes per second
    uint64_t tsLast;
    uint32_t bytesLast;
};
