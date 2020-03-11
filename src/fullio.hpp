#pragma once

std::pair<bool,uint32_t> full_read (int fd, void *buf, uint32_t size);
uint32_t full_write (int fd, const void *buf, uint32_t size);
