R"(////////// SHADER BEGIN /////////
//  LightShader.vert
//  Cornell University Game Library (CUGL)
//
// This is code from a positional light shader from Kalle Hameleinen's 
// box2dLights (version 1.5). Box2dLights is distributed under the Apache 
// license. For the original source code, refer to 
//
//    https://github.com/libgdx/box2dlights
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
		
// The attributes
in vec4  aPosition;
in vec4  aColor;
in float aScale;

out vec4 outColor;

// The projection matrix
uniform mat4 uProjection;

// Whether to use premultipled alpha
uniform int uPremultiplied;

// Transform and scale color
void main() {
    if (uPremultiplied > 0) {
        outColor = aScale * aColor;
    } else {
        outColor = aColor;
        outColor.a *= aScale;
    }
    gl_Position = uProjection * aPosition;
}

/////////// SHADER END //////////)"

