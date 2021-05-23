#pragma once

namespace mod::infinite_pit {

class RandomizerStrings {
public:
    // Looks up whether there is a replacement string for a given msgSearch key.
    static const char* LookupReplacement(const char* msg_key);
};
 
}