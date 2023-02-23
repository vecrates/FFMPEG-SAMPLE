attribute vec4 position;
attribute vec2 inputTextureCoordinate;
varying vec2 textureCoordinate;
uniform mat4 vertexMatrix;
uniform mat4 textureMatrix;
void main()
{
    gl_Position = vertexMatrix * position;
    textureCoordinate = (textureMatrix * vec4(inputTextureCoordinate, 0.0, 1.0)).xy;
}