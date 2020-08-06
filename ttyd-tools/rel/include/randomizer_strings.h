#pragma once

namespace mod::pit_randomizer {

class RandomizerStrings {
public:
    // Looks up whether there is a replacement string for a given msgSearch key.
    static const char* LookupReplacement(const char* msg_key);
};
 
}