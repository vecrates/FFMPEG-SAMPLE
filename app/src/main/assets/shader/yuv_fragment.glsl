precision highp float;
            
varying  vec2 textureCoordinate;
uniform sampler2D yTexture;
uniform sampler2D uTexture;
uniform sampler2D vTexture;

void main() {
    float y = texture2D(yTexture, textureCoordinate).r; //[0,1]
    float u = texture2D(uTexture, textureCoordinate).r - 0.5; //[-0.5~,0.5]
    float v = texture2D(vTexture, textureCoordinate).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344 * u - 0.714 * v;
    float b = y + 1.772 * u;

    gl_FragColor = vec4(r, g, b, 1.0);
}
