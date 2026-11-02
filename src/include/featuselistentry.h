#pragma once

// One feat's daily-use record: the server stats count the uses spent today, the client mirror the
// uses it was told are left.
struct CFeatUseListEntry {
    unsigned short m_nFeat;
    unsigned char  m_nUses;
};
