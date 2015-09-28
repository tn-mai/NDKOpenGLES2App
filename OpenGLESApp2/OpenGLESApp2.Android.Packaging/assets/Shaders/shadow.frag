#version 100

precision highp float;

varying vec3 pos;

highp vec2 frexp(highp float x)
{
	highp float e = ceil(log2(x));
	return vec2(x * exp2(-e), e * (-1.0 / 255.0));
}

void main()
{
    /* Generate shadow map - write fragment depth. */
	const float coef = 1.0 / 255.0;
	const float near = 10.0;
	const float far = 200.0;
	const float linerDepth = 1.0 / (far - near);
	float len = min(length(pos) * linerDepth, 1.0);
	vec4 r;

#if 1
	r.x = len;
	r.y = fract(r.x * 255.0);
	r.x -= r.y * coef;
#else
	r.xy = frexp(len);
#endif

#if 1
	r.z = len * len;
	r.w = fract(r.z * 255.0);
	r.z -= r.w * coef;
#else
	r.zw = frexp(len * len);
#endif
	gl_FragColor = r;
}
