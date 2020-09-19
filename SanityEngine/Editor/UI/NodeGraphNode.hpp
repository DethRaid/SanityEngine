#pragma once
#include "core/types.hpp"

namespace Sanity::Editor::UI {
    /*!
     * \brief A node in a node graph. Can be used to implement a material, AI logic, density map generation... many wonderful things
     */
    class NodeGraphNode {
    public:
        /*!
         * \brief Draws the node
         */
        void draw();

    protected:
        /*!
         * \brief Draws the actual node type. Subclasses should override this to provide the contents of a node
         */
        virtual void draw_self() = 0;

    private:
        inline static Int32 unique_id = 0;
    };
} // namespace Sanity::Editor::UI
