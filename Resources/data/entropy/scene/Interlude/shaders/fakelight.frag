#version 450
#define LIGHT_NUM 5
uniform vec3 fakelights[LIGHT_NUM];
in vec4 t_position;
out vec4 fragColor;
void main(void) {
	float a = 0.0;
	for ( int i=0; i<LIGHT_NUM; i++ ) {
		a += clamp((300.0 - distance(t_position.xyz, fakelights[i]) ) / 200.0, 0.0, 1.0);
		a = clamp(a, 0.0, 1.0);
	}
	
	

	fragColor = vec4(1.0, 1.0, 1.0, a);
}
