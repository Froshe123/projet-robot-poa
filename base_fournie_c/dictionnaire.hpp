#ifndef DICTIONNAIRE_HPP
#define DICTIONNAIRE_HPP

#include <string>
#include "json.hpp"

// ce que le dictionnaire a trouve dans un message
struct Emotion {
    bool trouvee = false; // false = "emotion indeterminee"
    std::string emotion; // ex "tristesse"
    std::string intensite; // ex "moyenne"
};

const JsonValue *chercher_mot(const JsonValue &dictionnaire, const std::string &mot);
Emotion lire_emotion(const JsonValue &dictionnaire, const std::string &message);

#endif
