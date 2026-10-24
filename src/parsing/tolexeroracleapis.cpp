#include "parsing/tolexeroracleapis.h"

#include "core/persistenttrie.h"
#include "parsing/tolexeroracle.h"
#include "core/toconnection.h"
#include "core/tocache.h"
#include "widgets/totoolwidget.h"

#include <algorithm>
#include <QtCore/QMap>

toLexerOracleAPIs::toLexerOracleAPIs(QsciLexer *lexer) : QsciAbstractAPIs(lexer)
{
};

toLexerOracleAPIs::~toLexerOracleAPIs()
{
};

void toLexerOracleAPIs::updateAutoCompletionList(const QStringList &context, QStringList &list)
{
    list << "AAA" << "BBB";
    QsciScintilla* editor = lexer()->editor();
    if (editor == NULL)
        return;

    QmlJS::PersistentTrie::Trie trie;

    QString text = editor->text();
    int len = editor->length();
    char* bufferText = new char[len+1];
    editor->SendScintilla(QsciScintillaBase::SCI_GETTEXT, len, bufferText);
    bufferText[len] = '\0';
    std::unique_ptr <SQLLexer::Lexer> lexer = LexerFactTwoParmSing::Instance().create("OracleGuiLexer", "", "toCustomLexer");
    lexer->setStatement(bufferText, len);
    SQLLexer::Lexer::token_const_iterator it = lexer->begin();
    while (it != lexer->end())
    {
        SQLLexer::Token::TokenType t = it->getTokenType();
        if (it->getTokenType() == SQLLexer::Token::L_IDENTIFIER)
            trie.insert(it->getText());
        it++;
    }
    list << trie.complete(context.last());
}

// Return the call tip for a function/procedure/package member, resolved via
// the schema metadata cache (same cache the dot-triggered completion uses).
// `context` is a dot-separated identifier chain built by QsciScintilla's own
// apiContext(), with a trailing empty placeholder - e.g. for
// "SCOTT.MY_PKG.MY_PROC(" it is ["SCOTT", "MY_PKG", "MY_PROC", ""].
QStringList toLexerOracleAPIs::callTips(const QStringList &context, int commas,
                                        QsciScintilla::CallTipsStyle style,
                                        QList<int> &shifts)
{
    QStringList cts;

    QsciScintilla* editor = lexer()->editor();
    if (editor == NULL)
        return cts;

    toConnection &connection = toConnection::currentConnection(editor);
    if (!connection.providerIs("Oracle"))
        return cts; // ALL_ARGUMENTS is Oracle-specific; this class is shared by other lexers

    // Drop the trailing partial-word placeholder apiContext()/callTip() always append.
    QStringList parts = context;
    if (!parts.isEmpty())
        parts.removeLast();

    if (parts.isEmpty() || parts.size() > 3)
        return cts; // bare call with no name, or something we don't try to resolve

    QString const currentSchema = toToolWidget::currentSchema(editor);
    QString owner, containerName, procName;

    if (parts.size() == 1)
    {
        // PROC(
        procName = parts.at(0);
        containerName = procName;
    }
    else if (parts.size() == 2)
    {
        // Ambiguous: SCHEMA.PROC( or PACKAGE.PROC(
        procName = parts.at(1);
        QStringList owners = connection.getCache().userList(toCache::OWNERS);
        if (owners.contains(parts.at(0).toUpper()))
        {
            owner = parts.at(0).toUpper();
            containerName = procName;
        }
        else
        {
            containerName = parts.at(0);
        }
    }
    else // parts.size() == 3
    {
        // SCHEMA.PACKAGE.PROC(
        owner = parts.at(0).toUpper();
        containerName = parts.at(1);
        procName = parts.at(2);
    }

    toCache::ObjectRef ref;
    ref.context = currentSchema;
    ref.first = owner;
    ref.second = containerName.toUpper();

    toCache::CacheEntry const* entry = connection.getCache().findEntry(ref);
    if (!entry || (entry->type != toCache::PROCEDURE && entry->type != toCache::FUNCTION && entry->type != toCache::PACKAGE))
        return cts;

    entry = connection.getCache().describeEntry(entry);
    if (!entry)
        return cts;

    toQArgumentDescriptionList args = entry->description.value("ARGUMENTLIST").value<toQArgumentDescriptionList>();
    if (args.isEmpty())
        return cts;

    // Group by overload, keep only the member actually being called, in position order.
    QMap<int, QList<toCache::ArgumentDescription> > byOverload;
    foreach(toCache::ArgumentDescription const& a, args)
    {
        if (a.ObjectName.compare(procName, Qt::CaseInsensitive) == 0)
            byOverload[a.Overload].append(a);
    }

    foreach(int overload, byOverload.keys())
    {
        QList<toCache::ArgumentDescription> params = byOverload.value(overload);
        std::sort(params.begin(), params.end(),
                  [](toCache::ArgumentDescription const& a, toCache::ArgumentDescription const& b)
                  { return a.Position < b.Position; });

        QStringList paramStrings;
        foreach(toCache::ArgumentDescription const& a, params)
        {
            if (a.Name.isEmpty()) // position 0 == function return type pseudo-row
                continue;
            QString p = a.Name + ' ' + a.InOut + ' ' + a.Datatype;
            if (a.Defaulted)
                p += " := default";
            paramStrings << p;
        }
        cts << (procName.toUpper() + '(' + paramStrings.join(", ") + ')');
    }

    return cts;
}
