NOMBRE: Arnau González Vilar
NIA: 218172
MAIL: arnau.gonzalez02@estudiant.upf.edu

NOMBRE: Carles Canals Gascon
NIA: 196298
MAIL: carles.canals01@estudiant.upf.edu

DESCRIPCIÓN DE OPCIONES:
La aplicación permite los siguientes modos de render seleccionables en el ImGui:

	- DEFERRED
	- FORWARD

La aplicación permite dos modos de iluminación en FORWARD: mutipass (MULTI) y singlepass (SINGLE). En DEFERRED no hay más opciones en este sentido.

Por otra parte, respecto a las ecuaciones de la luz hay tres opciones, donde las intensidades que se utilizan para la escena se han indicado en base a usar la opción DIRECT_BURLEY, donde si se ua DIRECT_LAMB el cambio es mínimo pero al usar PHONG la imagen se quema mucho, con lo que se tendrían que cambiar las intensidades de las luces. Las opciones son:

	- PHONG
	- DIRECT_LAMB
	- DIRECT_BURLEY

También permite usar un algoritmo de PCF para suavizar las sombras y hay una opción para activar un tonemapper para un render HDR donde tenemos disponibles las siguientes opciones:

	- HDR Scale
	- HDR Average Luminance
	- HDR White Intensity
	- HDR Gamma Correction

Se permite visualizar el depth buffer de las cámaras de las diferentes luces. 

Las anteriores opciones estan indicadas para ser activadas y desactivadas en el ImGui, pero para cambiar entre las diferentes luces en el depth viewport, pulsa 1.

Además se ha incluido un modo de calidad que determina (por el momento) la resolución de los depth buffers en los shadowmaps. Se escoge en el ImGui y por defecto viene en MEDIUM, dónde las opciones son:
	
	- LOW: 1024x1024
	- MEDIUM: 2048x2048
	- HIGH: 4096x4096
	- ULTRA: 8192x8192

Todos los parámetros que se indican en el shadowmap se pueden cambiar libremente con ImGui. 

Además, añadir que al usar PCF lo más probable es que haya que aumentarlo respecto los valores que vienen indicados en scene.json para obtener un resultado satisfactorio.

Por otra parte, exclusivamente de DEFERRED hay dos opciones más, siendo la primera para activar el dithering para los materiales con BLEND dónde si no se activa se hace una pasada de forward para estos y la opción de usar SSAO para oclusión ambiental, pudiendo a la vez indicar si se usa SSAO+ y si se hace un blur del resultado final. Además, se pueden variar lo siguiente parámetros:

	- SSAO Factor
	- SSAO Samples
	- SSAO Bias

Por último, pulsando 2 se pueden visualizar los gbuffers de albedo, depth, normales y emisiva; donde pulsando 3 mientras se tiene activa la visualización de gbuffers se pasa a ver la metallic, roughness, AO texture y, si está activa, el resutlado de SSAO.