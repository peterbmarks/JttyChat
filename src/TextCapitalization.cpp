#include "TextCapitalization.h"

#include <QRegularExpression>

namespace {

// A loose "does this look like a callsign" heuristic rather than a strict
// validator: an alphanumeric run, 3-7 characters, with at least one letter
// and at least one digit. This also happily covers grid locators (e.g.
// "FN42"), which are conventionally written upper case too.
bool looksLikeCallsign(const QString &run)
{
    static const QRegularExpression pattern(
        QStringLiteral("^(?=[A-Z0-9]*[A-Z])(?=[A-Z0-9]*[0-9])[A-Z0-9]{3,7}$"));
    return pattern.match(run).hasMatch();
}

bool containsLetter(const QString &run)
{
    for (QChar c : run) {
        if (c.isLetter())
            return true;
    }
    return false;
}

} // namespace

namespace TextCapitalization {

QString apply(const QString &text)
{
    QString result;
    result.reserve(text.size());

    // True until the first letter is written, then again after a '.', '!'
    // or '?'. A run of digits alone (e.g. a leading "599") doesn't use up
    // the opportunity to capitalize the next actual word.
    bool atSentenceStart = true;

    int i = 0;
    const int n = text.size();
    while (i < n) {
        const QChar c = text[i];

        if (c.isSpace()) {
            result.append(c);
            ++i;
            continue;
        }

        if (c.isLetterOrNumber()) {
            const int start = i;
            while (i < n && text[i].isLetterOrNumber())
                ++i;
            const QString run = text.mid(start, i - start);

            if (looksLikeCallsign(run.toUpper())) {
                result.append(run.toUpper());
                if (containsLetter(run))
                    atSentenceStart = false;
            } else {
                QString lowered = run.toLower();
                if (atSentenceStart) {
                    for (QChar &lc : lowered) {
                        if (lc.isLetter()) {
                            lc = lc.toUpper();
                            atSentenceStart = false;
                            break;
                        }
                    }
                }
                result.append(lowered);
            }
            continue;
        }

        // Punctuation (or anything else) between/within words: copy
        // verbatim, and treat '.', '!' or '?' as ending the sentence.
        const int start = i;
        while (i < n && !text[i].isSpace() && !text[i].isLetterOrNumber())
            ++i;
        const QString run = text.mid(start, i - start);
        result.append(run);
        if (run.contains(QLatin1Char('.')) || run.contains(QLatin1Char('!'))
            || run.contains(QLatin1Char('?'))) {
            atSentenceStart = true;
        }
    }

    return result;
}

} // namespace TextCapitalization
