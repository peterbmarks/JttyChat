#pragma once

#include <QString>

// Reformats JTTY message text (which the codec normalizes to all upper
// case) for friendlier display: sentence capitalization, with words that
// look like radio callsigns (or similarly shaped tokens like grid
// locators - e.g. "VK3TPM", "FN42") left upper case. Purely a display
// transform; never applied to text before encoding/transmission.
namespace TextCapitalization {

QString apply(const QString &text);

} // namespace TextCapitalization
