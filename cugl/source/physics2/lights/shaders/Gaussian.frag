R"(////////// SHADER BEGIN /////////
//  Gaussian.frag
//  Cornell University Game Library (CUGL)
//
//  This is code a revision of the the gaussian shader from Kalle Hameleinen's
//  box2dLights (version 1.5). Box2dLights is distributed under the Apache
//  license. For the original source code, refer to
//
//    https://github.com/libgdx/box2dlights
//
//  This version is a cleaner implementation of a two pass Gaussian that uses
//  a standard kernel.
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

// The data from the vertex shader
in vec2 outTexCoord;

// The texture to sample
uniform sampler2D uTexture;
// The texel size of a pixel (1.0/width, 1.0/height)
uniform vec2 uTexelSize;
// The direction for this gaussian pass
uniform vec2 uDirection;

void main() {
    // 9-tap-ish Gaussian weights (sigma ~2), stored as symmetric half-kernel.
    float w[5] = float[](
        0.227027,
        0.1945946,
        0.1216216,
        0.054054,
        0.016216
    );

    vec2 stepUV = uDirection * uTexelSize;
    
    vec4 sum = texture(uTexture, outTexCoord) * w[0];
    for (int ii = 1; ii < 5; ++ii) {
        vec2 o = stepUV * float(ii);
        sum += texture(uTexture, outTexCoord + o) * w[ii];
        sum += texture(uTexture, outTexCoord - o) * w[ii];
    }

    frag_color = sum;
}

/////////// SHADER END //////////)"
