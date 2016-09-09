#version 450
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
in vec4 position;
out vec4 t_position;
void main(void)
{
	vec4 eyeCoord = modelViewMatrix * position;

	t_position = position;
	gl_Position = projectionMatrix * eyeCoord;
}
