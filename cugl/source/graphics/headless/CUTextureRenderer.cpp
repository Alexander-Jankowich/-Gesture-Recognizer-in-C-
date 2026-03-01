//
//  CUTextureRendererVulkan.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a way to render a texture to the screen. It is a simple 
//  fullscreen quad pass. In the headless backend, this will store data for the
//  shader, but will not actually compute anything. This allows the headless
//  mode to be used for non-graphical simulations.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Zachary Schecter, Walker White
//  Version: 12/1/25 (Vulkan Integration)
//
#include <cugl/graphics/CUTextureRenderer.h>
#include <cugl/graphics/CUGraphicsShader.h>
#include <cugl/graphics/CUTexture.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

/**
 * Draws a full screen quad given a texture
 *
 * @param texture The texture to draw
 */
void TextureRenderer::draw(const std::shared_ptr<Texture>& texture) {
    // No-op in headless
}

/**
 * Initializes a texture renderer.
 *
 * @return true if initialization was successful
 */
bool TextureRenderer::init() {
    ShaderSource vsource;
    ShaderSource fsource;
    _shader = GraphicsShader::alloc(vsource,fsource);
    
    ResourceDef def;
    def.type = ResourceType::TEXTURE;
    def.set  = 0;
    def.location = 0;
    def.stage = ShaderStage::FRAGMENT;
    def.arraysize = 1;
    _shader->declareResource("uTexture", def);
    
    return _shader->compile();
}
