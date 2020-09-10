#pragma once

/*!
 * \brief Interface for a game
 */
class [[sanity::runtime_class]] IGame {
public:
    virtual void init() = 0;

};
