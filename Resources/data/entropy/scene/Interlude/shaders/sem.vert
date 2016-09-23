#version 450


uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
in vec4 position;
in vec3 normal;

out vec3 e;
out vec3 n;

void main() {

	e = normalize( vec3( projectionMatrix * modelViewMatrix * vec4( position.xyz, 1.0 ) ) );
    //e *= -1.0;
    mat3 m3 = mat3(modelViewMatrix);
    m3 = transpose(inverse(m3));
	n = normalize( m3 * normal ).xyz;

	gl_Position = projectionMatrix * modelViewMatrix * position;

}