#pragma once

#include <QString>

namespace latex {

// Translates a LaTeX math expression into rich text HTML.
// display=true renders \\ as a line break and is used for $$...$$ blocks.
QString renderToHtml(const QString &source, bool display);

// Replaces $...$ in an HTML fragment with rendered math, leaving <code> and
// <pre> spans untouched so shell variables like $PATH survive.
QString renderMathInHtml(const QString &html);

}  // namespace latex
