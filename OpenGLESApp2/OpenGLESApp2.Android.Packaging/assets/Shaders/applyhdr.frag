uniform sampler2D texSource[3]; // 0:main color, 1:1/4 color, 2:hdr factor.

varying mediump vec4 texCoord; // xy for main. zw for other.

void main()
{
  gl_FragColor = texture2D(texSource[0], texCoord.xy);
  gl_FragColor.rgb += texture2D(texSource[2], texCoord.zw).rgb;
}
