#include "plugins/chat/lib/LatexRenderer.hpp"

#include <QHash>
#include <QRegularExpression>
#include <QStringList>

namespace {

QString u(const char *utf8) {
    return QString::fromUtf8(utf8);
}

QString escapeHtml(const QString &text) {
    QString out = text;
    out.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    out.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    out.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    return out;
}

const QHash<QString, QString> &symbols() {
    static const QHash<QString, QString> table = {
        // lowercase greek
        {"alpha", u("\u03B1")},   {"beta", u("\u03B2")},    {"gamma", u("\u03B3")},
        {"delta", u("\u03B4")},  {"epsilon", u("\u03B5")}, {"zeta", u("\u03B6")},
        {"eta", u("\u03B7")},     {"theta", u("\u03B8")},  {"iota", u("\u03B9")},
        {"kappa", u("\u03BA")},   {"lambda", u("\u03BB")}, {"mu", u("\u03BC")},
        {"nu", u("\u03BD")},      {"xi", u("\u03BE")},     {"pi", u("\u03C0")},
        {"rho", u("\u03C1")},     {"sigma", u("\u03C3")},  {"tau", u("\u03C4")},
        {"upsilon", u("\u03C5")}, {"phi", u("\u03C6")},    {"chi", u("\u03C7")},
        {"psi", u("\u03C8")},     {"omega", u("\u03C9")},
        // uppercase greek
        {"Gamma", u("\u0393")},  {"Delta", u("\u0394")},  {"Theta", u("\u0398")},
        {"Lambda", u("\u039B")}, {"Xi", u("\u039E")},     {"Pi", u("\u03A0")},
        {"Sigma", u("\u03A3")},  {"Upsilon", u("\u03A5")},{"Phi", u("\u03A6")},
        {"Psi", u("\u03A8")},    {"Omega", u("\u03A9")},
        // operators and relations
        {"times", u("\u00D7")},   {"div", u("\u00F7")},    {"cdot", u("\u00B7")},
        {"pm", u("\u00B1")},      {"mp", u("\u2213")},     {"leq", u("\u2264")},
        {"le", u("\u2264")},      {"geq", u("\u2265")},    {"ge", u("\u2265")},
        {"neq", u("\u2260")},     {"ne", u("\u2260")},     {"approx", u("\u2248")},
        {"equiv", u("\u2261")},   {"sim", u("\u223C")},    {"simeq", u("\u2243")},
        {"propto", u("\u221D")},  {"ll", u("\u226A")},     {"gg", u("\u226B")},
        {"subset", u("\u2282")},  {"supset", u("\u2283")}, {"subseteq", u("\u2286")},
        {"in", u("\u2208")},      {"notin", u("\u2209")},  {"ni", u("\u220B")},
        {"cup", u("\u222A")},     {"cap", u("\u2229")},    {"emptyset", u("\u2205")},
        {"varnothing", u("\u2205")}, {"forall", u("\u2200")}, {"exists", u("\u2203")},
        {"neg", u("\u00AC")},     {"lnot", u("\u00AC")},   {"land", u("\u2227")},
        {"lor", u("\u2228")},     {"wedge", u("\u2227")},  {"vee", u("\u2228")},
        // arrows
        {"rightarrow", u("\u2192")}, {"to", u("\u2192")},  {"leftarrow", u("\u2190")},
        {"gets", u("\u2190")},    {"leftrightarrow", u("\u2194")}, {"Rightarrow", u("\u21D2")},
        {"Leftarrow", u("\u21D0")}, {"Leftrightarrow", u("\u21D4")}, {"implies", u("\u21D2")},
        {"iff", u("\u27FA")},     {"mapsto", u("\u21A6")},
        // misc symbols
        {"infty", u("\u221E")},   {"partial", u("\u2202")}, {"nabla", u("\u2207")},
        {"ldots", u("\u2026")},   {"dots", u("\u2026")},   {"cdots", u("\u22EF")},
        {"prime", u("\u2032")},   {"degree", u("\u00B0")}, {"angle", u("\u2220")},
        {"perp", u("\u22A5")},    {"parallel", u("\u2225")}, {"circ", u("\u2218")},
        {"ast", u("\u2217")},     {"star", u("\u22C6")},   {"hbar", u("\u0127")},
        {"ell", u("\u2113")},     {"Re", u("\u211C")},     {"Im", u("\u2111")},
        {"wp", u("\u2118")},      {"imath", u("\u0131")},  {"jmath", u("\u0237")},
        {"checkmark", u("\u2713")}, {"square", u("\u25A1")}, {"triangle", u("\u25B3")},
        // large operators
        {"sum", u("\u2211")},     {"prod", u("\u220F")},   {"coprod", u("\u2210")},
        {"int", u("\u222B")},     {"iint", u("\u222C")},   {"iiint", u("\u222D")},
        {"oint", u("\u222E")},    {"bigcup", u("\u22C3")}, {"bigcap", u("\u22C2")},
        {"bigoplus", u("\u2A01")}, {"bigotimes", u("\u2A02")}, {"lim", u("lim")},
        // function names (no trailing backslash in LaTeX either)
        {"log", u("log")},        {"ln", u("ln")},         {"exp", u("exp")},
        {"sin", u("sin")},        {"cos", u("cos")},       {"tan", u("tan")},
        {"cot", u("cot")},        {"sec", u("sec")},       {"csc", u("csc")},
        {"sinh", u("sinh")},      {"cosh", u("cosh")},     {"tanh", u("tanh")},
        {"arcsin", u("arcsin")},  {"arccos", u("arccos")}, {"arctan", u("arctan")},
        {"min", u("min")},        {"max", u("max")},       {"sup", u("sup")},
        {"inf", u("inf")},        {"arg", u("arg")},       {"det", u("det")},
        {"dim", u("dim")},        {"deg", u("deg")},       {"gcd", u("gcd")},
        {"ker", u("ker")},        {"mod", u("mod")},       {"bmod", u("mod")},
        // blackboard bold
        {"R", u("\u211D")},       {"N", u("\u2115")},      {"Z", u("\u2124")},
        {"Q", u("\u211A")},       {"C", u("\u2102")},      {"P", u("\u2119")},
        // escaped literals
        {"%", u("%")},            {"{", u("{")},           {"}", u("}")},
        {"$", u("$")},            {"&", u("&")},           {"#", u("#")},
        {"_", u("_")},
    };
    return table;
}

// spaces
const QHash<QString, QString> &spacing() {
    static const QHash<QString, QString> table = {
        {",", u("\u2009")}, {":", u("\u2005")}, {";", u("\u2005")}, {"!", u("")},
        {"quad", u("\u2003")}, {"qquad", u("\u2003\u2003")},
        {"thinspace", u("\u2009")}, {"enspace", u("\u2002")}, {"space", u(" ")},
    };
    return table;
}

// text-mode fonts we can flatten
bool isFontCommand(const QString &name) {
    return name == QLatin1String("text") || name == QLatin1String("textbf")
        || name == QLatin1String("textit") || name == QLatin1String("mathrm")
        || name == QLatin1String("mathbf") || name == QLatin1String("mathit")
        || name == QLatin1String("mathcal") || name == QLatin1String("mathbb")
        || name == QLatin1String("mathsf") || name == QLatin1String("mathtt")
        || name == QLatin1String("operatorname") || name == QLatin1String("mbox");
}

bool isAccentCommand(const QString &name) {
    return name == QLatin1String("vec") || name == QLatin1String("hat")
        || name == QLatin1String("bar") || name == QLatin1String("tilde")
        || name == QLatin1String("dot") || name == QLatin1String("ddot")
        || name == QLatin1String("hat") || name == QLatin1String("overline")
        || name == QLatin1String("widehat");
}

bool isEnvironment(const QString &name) {
    return name == QLatin1String("matrix") || name == QLatin1String("pmatrix")
        || name == QLatin1String("bmatrix") || name == QLatin1String("vmatrix")
        || name == QLatin1String("cases") || name == QLatin1String("aligned")
        || name == QLatin1String("array") || name == QLatin1String("split");
}

class Parser {
public:
    Parser(const QString &source, bool display) : m_src(source), m_display(display) {
    }

    QString parse() {
        return parseSequence(false);
    }

private:
    QString parseSequence(bool untilClose) {
        QString out;
        QString pendingSpace;

        const auto flushSpace = [&] {
            if (!pendingSpace.isEmpty()) {
                out += escapeHtml(pendingSpace);
                pendingSpace.clear();
            }
        };

        while (m_pos < m_src.size()) {
            const QChar ch = m_src.at(m_pos);

            if (untilClose && ch == QLatin1Char('}'))
                break;

            if (ch == QLatin1Char('{')) {
                ++m_pos;
                flushSpace();
                out += parseSequence(true);
                continue;
            }

            if (ch == QLatin1Char('}') || ch == QLatin1Char('$')) {
                ++m_pos;
                continue;
            }

            if (ch == QLatin1Char('^') || ch == QLatin1Char('_')) {
                flushSpace();
                const bool isSup = (ch == QLatin1Char('^'));
                ++m_pos;
                out += isSup ? QStringLiteral("<sup>%1</sup>").arg(parseArgument())
                             : QStringLiteral("<sub>%1</sub>").arg(parseArgument());
                continue;
            }

            if (ch == QLatin1Char('\\')) {
                const QString handled = parseCommand();
                if (!handled.isEmpty())
                    flushSpace(), out += handled;
                else
                    pendingSpace = QStringLiteral(" ");
                continue;
            }

            if (ch.isSpace()) {
                ++m_pos;
                if (!pendingSpace.isEmpty() || !out.isEmpty())
                    pendingSpace = QStringLiteral(" ");
                continue;
            }

            flushSpace();
            ++m_pos;
            out += escapeHtml(QString(ch));
        }

        return out;
    }

    // reads one argument: a {group} or a single atom
    QString parseArgument() {
        skipSpace();
        if (m_pos >= m_src.size())
            return {};

        if (m_src.at(m_pos) == QLatin1Char('{')) {
            ++m_pos;
            return parseSequence(true);
        }

        if (m_src.at(m_pos) == QLatin1Char('\\')) {
            const QString handled = parseCommand();
            return handled.isEmpty() ? QStringLiteral(" ") : handled;
        }

        const QChar ch = m_src.at(m_pos);
        ++m_pos;
        return escapeHtml(QString(ch));
    }

    void skipSpace() {
        while (m_pos < m_src.size() && m_src.at(m_pos).isSpace())
            ++m_pos;
    }

    QString readCommandName() {
        ++m_pos;  // consume backslash
        QString name;
        while (m_pos < m_src.size() && m_src.at(m_pos).isLetter()) {
            name += m_src.at(m_pos);
            ++m_pos;
        }
        return name;
    }

    QString parseCommand() {
        if (m_pos + 1 >= m_src.size())
            return {};

        // \\ is a row separator inside environments and a line break outside them
        if (m_src.at(m_pos + 1) == QLatin1Char('\\')) {
            m_pos += 2;
            return m_display ? QStringLiteral("<br/>") : QStringLiteral(" ");
        }

        // escaped literal, e.g. \{ \% \$
        if (!m_src.at(m_pos + 1).isLetter())
            return escapeHtml(QString(m_src.at(++m_pos)));

        const QString name = readCommandName();
        if (name.isEmpty())
            return {};

        if (name == QLatin1String("frac") || name == QLatin1String("dfrac")
            || name == QLatin1String("tfrac") || name == QLatin1String("binom")) {
            const QString num = parseArgument();
            const QString den = parseArgument();
            const QString joiner = (name == QLatin1String("binom")) ? QStringLiteral(" over ")
                                                                    : QStringLiteral("/");
            return parenthesise(num) + joiner + parenthesise(den);
        }

        if (name == QLatin1String("sqrt")) {
            skipSpace();
            if (m_pos < m_src.size() && m_src.at(m_pos) == QLatin1Char('[')) {
                const int close = m_src.indexOf(QLatin1Char(']'), m_pos);
                if (close > 0) {
                    ++m_pos;
                    parseArgumentFrom(close);
                }
            }
            return u("\u221A") + parenthesise(parseArgument());
        }

        if (isFontCommand(name))
            return parseArgument();

        if (isAccentCommand(name)) {
            return QStringLiteral("<span style=\"text-decoration:overline\">%1</span>").arg(parseArgument());
        }

        if (name == QLatin1String("underbrace") || name == QLatin1String("overbrace")) {
            QString body = parseArgument();
            skipSpace();
            QString label;
            if (m_pos < m_src.size() && (m_src.at(m_pos) == QLatin1Char('_') || m_src.at(m_pos) == QLatin1Char('^')))
                label = parseSequenceLabel();
            return body + label;
        }

        if (name == QLatin1String("begin"))
            return parseEnvironment();

        if (name == QLatin1String("end"))
            return consumeGroup();

        if (name == QLatin1String("left") || name == QLatin1String("right") || name == QLatin1String("big")
            || name == QLatin1String("Big") || name == QLatin1String("bigg") || name == QLatin1String("Bigg")
            || name == QLatin1String("middle") || name == QLatin1String("displaystyle"))
            return {};

        if (spacing().contains(name))
            return spacing().value(name);

        if (symbols().contains(name))
            return symbols().value(name);

        return {};
    }

    // after \underbrace{..}_label, render the label as a sub/superscript
    QString parseSequenceLabel() {
        const bool isSup = m_src.at(m_pos) == QLatin1Char('^');
        ++m_pos;
        const QString label = parseArgument();
        return isSup ? QStringLiteral("<sup>%1</sup>").arg(label)
                     : QStringLiteral("<sub>%1</sub>").arg(label);
    }

    QString consumeGroup() {
        skipSpace();
        if (m_pos < m_src.size() && m_src.at(m_pos) == QLatin1Char('{')) {
            ++m_pos;
            return parseSequence(true);
        }
        return parseArgument();
    }

    void parseArgumentFrom(int closeIndex) {
        m_pos = closeIndex + 1;
    }

    QString parenthesise(const QString &fragment) const {
        const QString text = stripTags(fragment);
        if (text.size() <= 2 && !text.contains(QLatin1Char(' ')))
            return fragment;
        return QStringLiteral("(%1)").arg(fragment);
    }

    static QString stripTags(const QString &html) {
        QString out;
        bool inTag = false;
        for (const QChar ch : html) {
            if (ch == QLatin1Char('<'))
                inTag = true;
            else if (ch == QLatin1Char('>'))
                inTag = false;
            else if (!inTag)
                out += ch;
        }
        return out;
    }

    QString parseEnvironment() {
        // \begin{name} body \end{name}
        skipSpace();
        if (m_pos >= m_src.size() || m_src.at(m_pos) != QLatin1Char('{'))
            return {};
        const int nameEnd = m_src.indexOf(QLatin1Char('}'), m_pos);
        if (nameEnd < 0)
            return {};

        QString name = m_src.mid(m_pos + 1, nameEnd - m_pos - 1).trimmed();
        m_pos = nameEnd + 1;

        // the body is everything up to the matching \end{...}; it is not brace-wrapped
        static const QRegularExpression endMarker(QStringLiteral("\\\\end\\s*\\{"));
        // endMarker already consumed the '{', so only the name and '}' remain
        static const QRegularExpression endName(QStringLiteral("[^}]*\\}"));

        const QRegularExpressionMatch end = endMarker.match(m_src, m_pos);
        if (!end.hasMatch())
            return {};

        const QString body = m_src.mid(m_pos, end.capturedStart() - m_pos);
        m_pos = end.capturedEnd();

        const QRegularExpressionMatch tail = endName.match(m_src, m_pos);
        if (tail.hasMatch())
            m_pos = tail.capturedEnd();

        QStringList rows;
        for (const QString &row : body.split(QStringLiteral("\\\\"))) {
            if (row.trimmed().isEmpty())
                continue;
            QString cells;
            for (const QString &cell : row.split(QLatin1Char('&')))
                cells += QStringLiteral("<td>%1</td>").arg(Parser(cell, m_display).parse());
            rows.append(cells);
        }

        QChar open;
        QChar close;
        if (name == QLatin1String("cases")) {
            open = u("{").at(0);
        } else if (name == QLatin1String("pmatrix") || name == QLatin1String("bmatrix")) {
            open = u("(").at(0);
            close = u(")").at(0);
        } else if (name == QLatin1String("vmatrix") || name == QLatin1String("Vmatrix")) {
            open = u("|").at(0);
            close = u("|").at(0);
        }

        QString html = QStringLiteral("<table cellspacing=\"0\" cellpadding=\"3\">");
        for (int i = 0; i < rows.size(); ++i) {
            html += QStringLiteral("<tr>");
            if (i == 0 && !open.isNull()) {
                html += QStringLiteral("<td rowspan=\"%1\" style=\"font-size:240%; padding-right:2px;\">%2</td>")
                            .arg(rows.size())
                            .arg(QString(open));
            }
            html += rows.at(i);
            if (i == 0 && !close.isNull()) {
                html += QStringLiteral("<td rowspan=\"%1\" style=\"font-size:240%; padding-left:2px;\">%2</td>")
                            .arg(rows.size())
                            .arg(QString(close));
            }
            html += QStringLiteral("</tr>");
        }
        html += QStringLiteral("</table>");

        return html;
    }

    QString m_src;
    int m_pos = 0;
    bool m_display = false;
};

QString replaceInlineMath(const QString &text) {
    // not $$ (those are handled as their own block), no newlines
    static const QRegularExpression math(
        QStringLiteral("(?<!\\$)\\$(?!\\$)((?:[^$\\n])+?)(?<!\\$)\\$(?!\\$)")
    );
    // a bare number is far more likely to be currency than math
    static const QRegularExpression currency(QStringLiteral("^[\\d.,\\s]+$"));

    QString out;
    int cursor = 0;

    for (auto it = math.globalMatch(text); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        const QString body = match.captured(1);
        if (currency.match(body).hasMatch())
            continue;

        out += text.mid(cursor, match.capturedStart() - cursor);
        out += Parser(body, false).parse();
        cursor = match.capturedEnd();
    }

    out += text.mid(cursor);
    return out;
}

}  // namespace

namespace latex {

QString renderToHtml(const QString &source, bool display) {
    if (source.trimmed().isEmpty())
        return {};

    QString html = Parser(source.trimmed(), display).parse();

    // LaTeX ignores spacing before punctuation, we do not
    static const QRegularExpression spaceBeforePunct(QStringLiteral(" +([,.;:])"));
    html.replace(spaceBeforePunct, QStringLiteral("\\1"));

    return html;
}

QString renderMathInHtml(const QString &html) {
    // never touch code spans: $PATH, $HOME, $(date) must survive verbatim
    static const QRegularExpression codeSpan(
        QStringLiteral("<(code|pre)\\b[^>]*>[\\s\\S]*?</\\1>"), QRegularExpression::CaseInsensitiveOption
    );

    QString out;
    int cursor = 0;

    for (auto it = codeSpan.globalMatch(html); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        out += replaceInlineMath(html.mid(cursor, match.capturedStart() - cursor));
        out += match.captured(0);
        cursor = match.capturedEnd();
    }
    out += replaceInlineMath(html.mid(cursor));

    return out;
}

}  // namespace latex
