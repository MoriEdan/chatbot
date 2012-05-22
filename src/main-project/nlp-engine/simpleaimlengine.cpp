/*
 * Copyright (C) 2012 Andres Pagliano, Gabriel Miretti, Gonzalo Buteler,
 * Nestor Bustamante, Pablo Perez de Angelis
 *
 * This file is part of LVK Botmaster.
 *
 * LVK Botmaster is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LVK Botmaster is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LVK Botmaster.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "simpleaimlengine.h"
#include "nlprule.h"

#include <exception>

//--------------------------------------------------------------------------------------------------
// SimpleAimlEngine::InvalidSyntaxException
//--------------------------------------------------------------------------------------------------

class Lvk::Nlp::SimpleAimlEngine::InvalidSyntaxException : public std::exception
{
public:
    InvalidSyntaxException(const QString &what) throw()
        : m_what(what.toUtf8())
    {
    }

    ~InvalidSyntaxException() throw() {}

    virtual const char* what() throw()
    {
        return m_what.constData();
    }

private:
    QByteArray m_what;
};

//--------------------------------------------------------------------------------------------------
// SimpleAimlEngine
//--------------------------------------------------------------------------------------------------
//
// TODO this class became too complex! Time to create our own engine and stop using AIML

#define VAR_NAME_REGEX  "\\[([A-Za-z_]+)\\]"
#define IF_REGEX        "\\{\\s*if\\s*" VAR_NAME_REGEX "\\s*=\\s*([^}]+)\\}" "([^{]+)"
#define ELSE_REGEX      "\\{\\s*else\\s*\\}(.+)"
#define IF_ELSE_REGEX   IF_REGEX ELSE_REGEX

#define KEYWORD_REGEX   "\\*\\*\\s*$"

#define STR_ERR_1       "A rule input cannot contain two or more variables"
#define STR_ERR_2       "Rules cannot contain two or more different variable names"


Lvk::Nlp::SimpleAimlEngine::SimpleAimlEngine()
    : AimlEngine()
{
    initRegexs();
}

//--------------------------------------------------------------------------------------------------

Lvk::Nlp::SimpleAimlEngine::SimpleAimlEngine(Sanitizer *sanitizer)
    : AimlEngine(sanitizer)
{
    initRegexs();
}

//--------------------------------------------------------------------------------------------------

void Lvk::Nlp::SimpleAimlEngine::initRegexs()
{
    QString localizedIfRegex = QString(IF_REGEX)
            .replace("if", QObject::tr("if"));

    QString localizedIfElseRegex  = QString(IF_ELSE_REGEX)
            .replace("if", QObject::tr("if"))
            .replace("else", QObject::tr("else"));

    m_varNameRegex = QRegExp(VAR_NAME_REGEX);
    m_ifRegex = QRegExp(localizedIfRegex);
    m_ifElseRegex = QRegExp(localizedIfElseRegex);
    m_keywordRegex = QRegExp(KEYWORD_REGEX);

    m_varNameRegex.setCaseSensitive(false);
    m_ifRegex.setCaseSensitive(false);
    m_ifElseRegex.setCaseSensitive(false);
    m_keywordRegex.setCaseSensitive(false);
}

//--------------------------------------------------------------------------------------------------

const Lvk::Nlp::RuleList & Lvk::Nlp::SimpleAimlEngine::rules() const
{
    return m_rules;
}

//--------------------------------------------------------------------------------------------------

Lvk::Nlp::RuleList & Lvk::Nlp::SimpleAimlEngine::rules()
{
    return m_rules;
}

//--------------------------------------------------------------------------------------------------

void Lvk::Nlp::SimpleAimlEngine::setRules(const Nlp::RuleList &rules)
{
    m_rules = rules;
    m_indexRemap.clear();

    RuleList newRules;
    convertToPureAiml(newRules);

    AimlEngine::setRules(newRules);
}

//--------------------------------------------------------------------------------------------------

void Lvk::Nlp::SimpleAimlEngine::convertToPureAiml(Nlp::RuleList &newRules)
{
    newRules.clear();

    foreach (const Nlp::Rule &rule, m_rules) {
        try {
            Lvk::Nlp::Rule newRule;
            convertToPureAiml(newRule, rule);
            newRules.append(newRule);
        } catch (std::exception &) {
            // Nothing to to. If the rule is invalid, we just skip it
            // TODO better error handling
        }
    }
}

//--------------------------------------------------------------------------------------------------

void Lvk::Nlp::SimpleAimlEngine::convertToPureAiml(Nlp::Rule &newRule, const Nlp::Rule &rule)
{
    ConvertionContext ctx;
    ctx.inputIdx = 0;
    ctx.rule = rule;

    newRule.setId(rule.id());
    newRule.setTarget(rule.target());
    convertInputList(newRule.input(), ctx);
    convertOutputList(newRule.output(), ctx);
}

//--------------------------------------------------------------------------------------------------

void Lvk::Nlp::SimpleAimlEngine::convertInputList(QStringList &inputList, ConvertionContext &ctx)
{
    /*
     * For each input, transform variables and keyword operators.
     * Both features are not supported in one single input
     */

    for (int i = 0; i < ctx.rule.input().size(); ++i) {
        ctx.input = ctx.rule.input().at(i);
        ctx.inputIdx = i;

        if (convertVariables(inputList, ctx)) {
            continue;
        }

        if (convertKeywordOp(inputList, ctx)) {
            continue;
        }

        // Already pure AIML

        inputList.append(ctx.input);
    }
}

//--------------------------------------------------------------------------------------------------

/*
 * Transform strings like:
 *    Do you like [VarName]
 *
 * To pure AIML:
 *    Do you like *
 */

bool Lvk::Nlp::SimpleAimlEngine::convertVariables(QStringList &inputList, ConvertionContext &ctx)
{
    int pos = m_varNameRegex.indexIn(ctx.input);

    // If variable decl found

    if (pos != -1) {

        if (pos != m_varNameRegex.lastIndexIn(ctx.input)) {
            throw InvalidSyntaxException(QObject::tr(STR_ERR_1));
        }
        if (!ctx.varName.isNull() && ctx.varName != m_varNameRegex.cap(1)) {
            throw InvalidSyntaxException(QObject::tr(STR_ERR_2));
        }

        ctx.varName = m_varNameRegex.cap(1);

        QString newInput = ctx.input;
        newInput.replace(m_varNameRegex, " * ");
        inputList.append(newInput);
    }

    return pos != -1;
}

//--------------------------------------------------------------------------------------------------

/*
 * Transform strings like:
 *    Footbal **
 *
 * To 4 pure AIML rules:
 *    Footbal
 *    _ Footbal
 *    Footbal _
 *    _ Footbal *
 */

bool Lvk::Nlp::SimpleAimlEngine::convertKeywordOp(QStringList &inputList, ConvertionContext &ctx)
{
    int pos = m_keywordRegex.indexIn(ctx.input);

    // if keyword operator found

    if (pos != -1) {
        QString baseInput = ctx.input;

        baseInput.remove(m_keywordRegex);

        inputList.append(baseInput);
        inputList.append(baseInput + " _");
        inputList.append("_ " + baseInput);
        inputList.append("_ " + baseInput + " *");

        RuleId id = ctx.rule.id();
        int size = inputList.size();

        m_indexRemap[id][size - 4] = ctx.inputIdx;
        m_indexRemap[id][size - 3] = ctx.inputIdx;
        m_indexRemap[id][size - 2] = ctx.inputIdx;
        m_indexRemap[id][size - 1] = ctx.inputIdx;
    }

    return pos != -1;
}


//--------------------------------------------------------------------------------------------------

void Lvk::Nlp::SimpleAimlEngine::convertOutputList(QStringList &outputList, ConvertionContext &ctx)
{
    outputList.clear();

    /*
     * For each output, convert strings like:
     *    {if [VarName] = football}
     *       Yes I like [VarName]
     *    {else}
     *       No, I don't
     *
     * To pure AIML:
     *    <think><set name="like"><star/></set></think>
     *    <condition>
     *       <li name="like" value="football"> Yes I like <star/></li>
     *       <li>No, I don't</li>
     *    </condition>
     */

    for (int i = 0; i < ctx.rule.output().size(); ++i) {

        // TODO refactor!

        QString output = ctx.rule.output().at(i);
        QString newOutput = output;
        int pos;

        // Parse If-Else

        pos = m_ifElseRegex.indexIn(output);
        if (pos != -1) {
            newOutput = QString("<condition>"
                                "<li name=\"%1\" value=\"%2\">%3</li>"
                                "<li>%4</li>"
                                "</condition>")
                                   .arg(m_ifElseRegex.cap(1))
                                   .arg(m_ifElseRegex.cap(2).trimmed())
                                   .arg(m_ifElseRegex.cap(3).trimmed())
                                   .arg(m_ifElseRegex.cap(4).trimmed());
        } else {
            // Parse If

            pos = m_ifRegex.indexIn(output);
            if (pos != -1) {
                newOutput = QString("<condition>"
                                    "<li name=\"%1\" value=\"%2\">%3</li>"
                                    "</condition>")
                                       .arg(m_ifRegex.cap(1))
                                       .arg(m_ifRegex.cap(2).trimmed())
                                       .arg(m_ifRegex.cap(3).trimmed());
            }
        }

        // Parse Variables

        pos = 0;
        while (pos != -1) {
            pos = m_varNameRegex.indexIn(output, pos);

            if (pos != -1) {
                QString varName = m_varNameRegex.cap(1);

                if (varName == ctx.varName) {
                    newOutput.replace("[" + varName + "]", "<star/>", Qt::CaseInsensitive);
                } else {
                    newOutput.replace("[" + varName + "]", "<get name=\"" + varName + "\" />",
                                      Qt::CaseInsensitive);
                }

                pos++;
            }

        }

        if (!ctx.varName.isNull()) {
            newOutput.prepend("<think>"
                              "<set name=\"" + ctx.varName + "\"><star/></set>"
                              "</think>");
        }

        outputList.append(newOutput);
    }
}

//--------------------------------------------------------------------------------------------------

QList<QString> Lvk::Nlp::SimpleAimlEngine::getAllResponses(const QString &input,
                                                           const QString &target,
                                                           Engine::MatchList &matches)
{
    QList<QString> responses = AimlEngine::getAllResponses(input, target, matches);

    remap(matches);

    return responses;
}

//--------------------------------------------------------------------------------------------------

void Lvk::Nlp::SimpleAimlEngine::remap(Engine::MatchList &matches)
{
    for (int i = 0; i < matches.size(); ++i) {
        Nlp::RuleId ruleId = matches[i].first;
        int inputIndex = matches[i].second;

        RuleIndexRemap::Iterator it = m_indexRemap.find(ruleId);
        if (it != m_indexRemap.end()) {
            IndexRemap::Iterator itt = it->find(inputIndex);
            if (itt != it->end()) {
                matches[i].second = itt.value();
            }
        }
    }
}
