R"(////////// SHADER BEGIN /////////
//  ShadowShader.frag
//  Cornell University Game Library (CUGL)
//
// This is code is an amalgamation of the diffuse, shadow, and without shadow
// shaders from Kalle Hameleinen's box2dLights (version 1.5). Box2dLights is
// distributed under the Apache license. For the original source code, refer to
//
//    https://github.com/libgdx/box2dlights
//
//  We combined these shaders because the metaprogramming features in those
//  shaders are not possible in Vulkan (because we are not embedding SpirV).
//  Therefore, we need to use uniforms any in these shaders. Therefore, it
//  makes sense to combined them.
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
//  Author: Walker White
//  Version: 12/20/25
//

#ifdef CUGLES
// This one line is all the difference
precision mediump float;
#endif

// The output color
out vec4 frag_color;

// The input from the vertex shader
in vec2 outTexCoord;

// The uniforms
uniform sampler2D uTexture;
uniform vec4 uAmbient;

// 0 for no-shadow, 1 for diffuse, 2 for shadow alpha, 3 for shadow premult
uniform int uType;

// Apply the lighting
void main() {
    if (uType == 0) {
        frag_color = texture(uTexture, outTexCoord);
    } else if (uType == 1) {
        frag_color.rgb = (uAmbient.rgb + texture(uTexture, outTexCoord).rgb);
        frag_color.a = 1.0;
    } else if (uType == 2) {
        vec4 color = texture(uTexture, outTexCoord);
        frag_color.rgb = color.rgb*color.a + uAmbient.rgb;
        frag_color.a = uAmbient.a - color.a;
    } else {
        vec4 color = texture(uTexture, outTexCoord);
        color.rgb += uAmbient.rgb;
        color.a = uAmbient.a - color.a;
        frag_color.rgb = color.rgb * color.a;
        frag_color.a = color.a;
    }
}

/////////// SHADER END //////////)"


