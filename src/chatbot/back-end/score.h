/*
 * Copyright (C) 2012 Andres Pagliano, Gabriel Miretti, Gonzalo Buteler,
 * Nestor Bustamante, Pablo Perez de Angelis
 *
 * This file is part of LVK Chatbot.
 *
 * LVK Chatbot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LVK Chatbot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LVK Chatbot.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LVK_BE_SCORE_H
#define LVK_BE_SCORE_H

#include <QDataStream>
#include <QMetaType>

namespace Lvk
{

/// \addtogroup Lvk
/// @{

namespace BE
{

/// \ingroup Lvk
/// \addtogroup BE
/// @{

/**
 * \brief The score struct provides an structure to store chabot scores.
 */
struct Score
{
    Score() : rules(0), contacts(0), conversations(0), total(0) { }

    double rules;         ///< Score obteined by rule definitions
    double contacts;      ///< Score obteined by chat contacts
    double conversations; ///< Score obteined by chat conversations
    double total;         ///< Total score
};


/**
 * Writes a \a rule to the stream and returns a reference to the stream.
 */
inline  QDataStream &operator<<(QDataStream &stream, const Score &score)
{
    return stream << score.rules << score.contacts << score.conversations << score.total;
}

/**
 * Reads a rule from the \a stream into \a rule, and returns a reference to the stream.
 */
inline QDataStream &operator>>(QDataStream &stream, Score &score)
{
    stream >> score.rules;
    stream >> score.contacts;
    stream >> score.conversations;
    stream >> score.total;
    return stream;
}

/**
 * Returns true if score \a s1 is equal than score \a s2. Otherwise returns false.
 */
inline bool operator==(const Score &s1, const Score &s2)
{
    return s1.rules == s2.rules &&
           s1.contacts == s2.contacts &&
           s1.conversations == s2.conversations &&
           s1.total == s2.total;
}

/**
 * Returns true if score \a s1 is different than score \a s2. Otherwise returns false.
 */
inline bool operator!=(const Score &s1, const Score &s2)
{
    return !(s1 == s2);
}

/// @}

} // namespace BE

/// @}

} // namespace Lvk


Q_DECLARE_METATYPE(Lvk::BE::Score)


#endif // LVK_BE_SCORE_H

