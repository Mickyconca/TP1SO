# TP1 SO - README

<hr>


## Autores

- [Roberto Catalan](https://github.com/rcatalan98)
- [María Victoria Conca](https://github.com/Mickyconca)
- [Gian Luca Pecile](https://github.com/glpecile).

##  Aplicación

La aplicación de resolución de problemas **SAT** consiste en los siguientes archivos:

  + **master**: crea los esclavos y envía los datos procesados a **vision** y mostrados **results.txt**.
  + **vision**: imprime los datos por *standard output*.
  + **slave**: realiza el procesamiento utilizando **minisat**.
  + **shm_lib** y **sem_lib**: librerías para el manejo de semáforos y memoria compartida.

## Compilación

Ejecutar `make` o `make all` para el compilado de los archivos. Asegurarse de tener instalado **minisat** con ``apt-get install minisat`` en su contenedor de docker.
Luego de la ejecución se generarán los siguientes archivos: 

  + **Master**
  + **Vision**
  + **Slave**

Si desea remover los mismos, ejecute `make clean` en el mismo directorio donde fue realizada la compilación.

## Ejecución

Para correr el programa ejecutar el archivo **Master** corriendo `./Master` junto a una lista de archivos **.cnf** pasados como parámetros. 

Por ejemplo: 

```bash
 ./Master files/*
```

Para ejecutar **Vision** hay dos formas:

1. Pipe de lo enviado por **Master** 

```bash
./Master files/* | ./Vision
```

2. Corriendo **Master** en una terminal y **Vision** en otra, pasando el valor retornado de Master.

```bash
# Terminal 1:
./Master
23
```

```bash
# Terminal 2:
 ./Vision 23
```

3. Correr **Master** en *background* y **Vision** en *foreground* pasando el valor de retorno de Master.

```bash
./solve files/*& # background
./vista 23       # foreground
```

Al finalizar la ejecución de **Master** se creará un archivo llamado **results.txt**, con la resolución de los **SAT** y por *standard output* será mostrado gracias a **Vision**.

Si desea remover los archivos generados, ejecute `make clean` en el mismo directorio donde fue realizada la compilación.

## Testeo

Para el testeo con tanto **PVS-Studio**, **Cppcheck** como **Valgrind** se debe correr el siguiente comando:

```bash
 make test
```

Los resultados se encontrarán de la siguiente manera:

 * **PVS-Studio:** *report.tasks*
 * **Valgrind:** *vision.valgrind*, *master.valgrind*
 * **Cppcheck:** *cppoutput.txt*

Para remover los mismos, correr el comando `make cleanTest` en el mismo directorio donde fue realizada la compilación.