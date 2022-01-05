/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#pragma once

#include "sre/impl/Export.hpp"

namespace sre {
    /**
     * Mesh topology used to define the kind of mesh
     */
    enum class MeshTopology {
		// Missing LineLoop -- need to find hex value? 0x0002?
        Points = 0x0000,
        Lines = 0x0001,
        LineStrip = 0x0003,
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        TriangleFan = 0x0006
    };
}
