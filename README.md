# Sunny Side Up
This project is my practice for NDK and OpenGL ES 2.0.
It is for Android.  
In addition for testing, it can execute on Windows using Google Angle.  
It include audio support by OpenSL ES for Android.  
and also contain the following technique:
- Image based lighting(IBL)/Pysical based rendering(PBR).
- Puseudo high dynamic range rendering(HDR).
- Exponential Shadow maps(ESM).
- Simple collision(Sphere vs Sphere, Sphere vs Plane, Sphere vs Rectangle).

This game controls by touch/swipe and tilt.

## About the release build.
When you want to build APK of the release version for publishing,  
you need to generate a keystore file, and to make a file named 'ant.properties' on 'OpenGLESApp2/OpenGLESApp2.Android.Packaging'.  
I does not publish them, because it contains the information that should kept secret.

If you are installing some version of Java, be careful to use keytool(the keystore generator) at the same version of Ant used by android SDK.  
Once published your app, you have to use a same keystore file continuously.

'ant.properties' is the text file that contains following:

  key.store=path/to/your/keystore  
  key.store.passward=passward/used/for/generating/your/keystore  
  key.alias=name/of/alias/used/for/generating/your/keystore  
  key.alias.password=password/of/alias/used/for/generating/your/keystore  

<img src="https://github.com/tn-mai/NDKOpenGLES2App/blob/master/ScreenShot/TitleNoon.jpg" width="50%" /><img src="https://github.com/tn-mai/NDKOpenGLES2App/blob/master/ScreenShot/MainGameSunset.jpg" width="50%" />
<img src="https://github.com/tn-mai/NDKOpenGLES2App/blob/master/ScreenShot/FailureNoon.jpg" width="50%" /><img src="https://github.com/tn-mai/NDKOpenGLES2App/blob/master/ScreenShot/SuccessNoon.jpg" width="50%" />
<img src="https://github.com/tn-mai/NDKOpenGLES2App/blob/master/ScreenShot/SuccessSunset.jpg" width="50%" />
