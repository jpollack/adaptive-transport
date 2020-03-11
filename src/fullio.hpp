#pragma once

std::pair<bool,size_t> full_read (int fd, void *buf, size_t size);
size_t full_write (int fd, const void *buf, size_t size);
