#version 330 core
// default 3f2f_pt frag

in vec2 TexCoord;
out vec4 color;

uniform sampler2D tex;

void main() {
    color = texture(tex, TexCoord);
}
