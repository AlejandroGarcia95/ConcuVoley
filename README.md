# ConcuVoley Checkpoints

> There are two kinds of development lifecycle. 
> There is the incremental/iterative way, which tends to work; and then everything else that is not incremental or iterative, which tends not to work.

Propongo seguir más o menos el siguiente flujo de trabajo para este TP.

1. Poder leer un archivo de configuración básico, con el que se puedan cargar los parámetros pedidos por el trabajo. Y tener una interfaz de log que te permita escribir mensajes en algún archivo de salida.
2. Implementar el TDA Jugador de Voley y darle funcionalidad para poder jugar con otro jugador. Usar esa funcionalidad para emular un partido 1 vs 1.
3. Convertir a cada jugador en un proceso, y modificar todo para poder emular un partido 1 vs 1 con tres procesos (padre y dos procesos hijos, uno por jugador).
4. Mejorar lo anterior para que el partido pueda ser 2 vs 2 como corresponde. 
5. Crear FxC canchas para emular FxC partidos lanzando 4 players por partido.
6. Mejorar lo anterior para agregar la playa, y que inicialmente los jugadores se queden ahí y pasen a jugar y sigan jugando. Agegar el sistema de puntos y salidas por log.
7. Agregar la marea para cambiar dinámicamente la cantidad de canchas disponibles y liquidar los partidos.

# Proposiciones

1. Los jugadores tienen un atributo numérico _skill_ y la duración de un partido es función de este parámetro.
2. En lugar de hacer que la marea suba o baje al apretar teclitas, se me ocurre que se puede guardar registro del paso del tiempo, y tener un archivo de texto secundario en el que se especifique cuándo sube o baja la marea a partir de timestamps. De esa manera, se puede controlar perfectamente la marea sin tener que apretar teclas durante la ejecución del programa, y si se quiere que sea random, se puede indicar con un flag en el archivo de configuración que así sea.

# Hipótesis

Rellenar con hipótesis que se vayan haciendo
