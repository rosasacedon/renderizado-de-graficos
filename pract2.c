/* Pract2  RAP 09/10    Javier Ayllon*/

#include <openmpi/mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h> 
#include <assert.h>   
#include <unistd.h>   
#include <math.h>
#include "mpi.h"

#define NIL (0) 
#define N 10 //numero de procesos que se inicializan
#define IMAGEN "foto.dat" //fichero del que obtenemos la imagen
#define TAM 400 //tamaño del fichero

/*Variables Globales */

XColor colorX;
Colormap mapacolor;
char cadenaColor[]="#000000";
Display *dpy;
Window w;
GC gc;

/*Funciones auxiliares */
int max(int num1, int num2) { //funcion para poder sacar el valor maximo de dos pixeles para resolver el filtro cianotipia
   int result;
   if (num1 > num2)
      result = num1;
   else
      result = num2;
   return result; //Ejercicios aprenderaprogramar.com
}

void initX() {

      dpy = XOpenDisplay(NIL);
      assert(dpy);

      int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
      int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

      w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 400, 400, 0, blackColor, blackColor);
      XSelectInput(dpy, w, StructureNotifyMask);
      XMapWindow(dpy, w);
      gc = XCreateGC(dpy, w, 0, NIL);
      XSetForeground(dpy, gc, whiteColor);
      for(;;) {
            XEvent e;
            XNextEvent(dpy, &e);
            if (e.type == MapNotify)
                  break;
      }
      mapacolor = DefaultColormap(dpy, 0);
}

void dibujaPunto(int x,int y, int r, int g, int b) {

        sprintf(cadenaColor,"#%.2X%.2X%.2X",r,g,b);
        XParseColor(dpy, mapacolor, cadenaColor, &colorX);
        XAllocColor(dpy, mapacolor, &colorX);
        XSetForeground(dpy, gc, colorX.pixel);
        XDrawPoint(dpy, w, gc,x,y);
        XFlush(dpy);
}

/* Programa principal */

int main (int argc, char *argv[]) {

    int rank,size;
    MPI_Comm commPadre, intercomm; //intercomunicador para que se comuniquen los procesos que tienen acceso a disco y los que tienen acceso a graficos
    MPI_Status status;
    int tag;
    int buf[5];
    int errCodes[N];
    unsigned char pixel[3];
    int numeroFilas = TAM / N;
    int tamBloque = numeroFilas * TAM * 3 * sizeof(unsigned char);
    int inicioLectura = rank * numeroFilas; 
    int finLectura = inicioLectura + numeroFilas; 

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_get_parent( &commPadre );
    
    if ((commPadre==MPI_COMM_NULL)&& (rank==0)) {
        initX();

        /* Codigo del maestro */

        //el proceso principal crea varios procesos trabajadores con el #define N 10
        MPI_Comm_spawn("pract2", &argv[1], N, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errCodes);

        /*En algun momento dibujamos puntos en la ventana algo como
        dibujaPunto(x,y,r,g,b);  */
        for(int i = 0; i < TAM*TAM; i++){ //proceso principal permanece en espera hasta que reciba los pixeles
            MPI_Recv(&buf, 5, MPI_INT, MPI_ANY_SOURCE, 0, intercomm, &status);
            dibujaPunto(buf[0], buf[1], buf[2], buf[3], buf[4]); //llama a la funcion dibujaPunto para pintar los pixeles en pantalla
        }
        sleep(10); //no cerrar inmediatamente la imagen completada
    }
    else {
        /* Codigo de todos los trabajadores */
        /* El archivo sobre el que debemos trabajar es foto.dat */

      //cada proceso abre una vista de la imagen correspondiente a la primera posicion que les toca leer del fichero
      MPI_File imagen;
      MPI_File_open(MPI_COMM_WORLD, IMAGEN, MPI_MODE_RDONLY, MPI_INFO_NULL, &imagen);
      MPI_File_set_view(imagen, tamBloque * rank, MPI_UNSIGNED_CHAR , MPI_UNSIGNED_CHAR, "native", MPI_INFO_NULL);
      
        if(rank == N-1){ //si es el ultimo proceso asignar lineas a los procesos
            finLectura = TAM - floor(TAM/N)*(N-1); //calcular lineas del ultimo proceso
        } else {	
	    finLectura = floor(TAM/N); //reparto de líneas entre otros procesos
	}

        for(int y = 0; y < finLectura; y++){ //recorrer eje y
            for(int x = 0; x < TAM; x++){ //recorrer el eje x

                MPI_File_read(imagen, pixel, 3, MPI_UNSIGNED_CHAR, &status); //lectura del fichero y paso de argumentos
                buf[0] = x;
                buf[1] = y + (rank * (ceil(TAM/N)));    

                switch(*argv[1]){

                    case 'B': /* BLUE */
                        buf[2] = 0;
                        buf[3] = 0;
                        buf[4] = (int)pixel[2];    
                        break;
                    case 'G': /* GREEN */
                        buf[2] = 0;
                        buf[3] = (int)pixel[1];
                        buf[4] = 0;
                        break;
                    case 'R': /* RED */
                        buf[2] = (int)pixel[0];
                        buf[3] = 0;
                        buf[4] = 0;  
                        break;
                    case 'W': /* BLACK & WHITE */
                        buf[2] = ((int)pixel[0] + (int)pixel[1] + (int)pixel[2]) / 3; 
                        buf[3] = ((int)pixel[0] + (int)pixel[1] + (int)pixel[2]) / 3;
                        buf[4] = ((int)pixel[0] + (int)pixel[1] + (int)pixel[2]) / 3;
                        break;		        
                    case 'S': /* SEPIA */
                        buf[2] = ((int)pixel[0] * 0.393) + ((int)pixel[1] * 0.769) + ((int)pixel[2] * 0.189); 
                        buf[3] = ((int)pixel[0] * 0.349) + ((int)pixel[1] * 0.686) + ((int)pixel[2] * 0.168); 
                        buf[4] = ((int)pixel[0] * 0.272) + ((int)pixel[1] * 0.534) + ((int)pixel[2] * 0.131); 
                        break;
                    case 'N': /* NEGATIVE */
                        buf[2] = 255-(int)pixel[0];
                        buf[3] = 255-(int)pixel[1];
                        buf[4] = 255-(int)pixel[2];
                        break;
                    case 'C': /* CIANOTIPIA SOBRE BLANCO */ 
                        buf[2] = 0;
                        buf[3] = (255-(int)pixel[0] + 255-(int)pixel[1] + 255-(int)pixel[2]) / 3; 
                        buf[4] = (255-(int)pixel[0] + 255-(int)pixel[1] + 255-(int)pixel[2]) / 3; 

                        if(buf[2] <= 150 && buf[3] <= 150 && buf[4] <= 150){
                            //si está por debajo de 150 son colores oscuros, de sombra
                            int temporal = max(buf[2], buf[3]);
                            temporal = max(temporal, buf[4]);
                            buf[2] = 255-temporal;
                            buf[3] = 255-temporal;
                            buf[4] = 255-temporal;
                        }
                        break;
                    case 'E': /* CIANOTIPIA SOBRE NEGRO */
                        buf[2] = 0;
                        buf[3] = (255-(int)pixel[0] + 255-(int)pixel[1] + 255-(int)pixel[2]) / 3; 
                        buf[4] = (255-(int)pixel[0] + 255-(int)pixel[1] + 255-(int)pixel[2]) / 3; 
                        break;
                    case 'O': /* CIANOTIPIA SOBRE ROJO */
                        buf[2] = ((int)pixel[0] + (int)pixel[1] + (int)pixel[2]) / 3; 
                        buf[3] = (255-(int)pixel[0] + 255-(int)pixel[1] + 255-(int)pixel[2]) / 3; 
                        buf[4] = (255-(int)pixel[0] + 255-(int)pixel[1] + 255-(int)pixel[2]) / 3; 
                        break;
                    default:
                        buf[2] = (int)pixel[0];
                        buf[3] = (int)pixel[1];
                        buf[4] = (int)pixel[2];
                        break;
                }
                for(int k = 2; k <= 10; k++){ //control para que no se salgan de los limites de los colores
                    if(buf[k] > 255){
                        buf[k] = 255; 
                    }else if (buf[k] < 0){
                        buf[k] = 0;
                    }
                }
       	    MPI_Bsend(&buf, 5, MPI_INT, 0, 0, commPadre); //se devuelve al proceso padre los pixeles con sus valores correspondientes al filtro
            }
	    }
        MPI_File_close(&imagen); //se cierra el fichero de la imagen
    }
    MPI_Finalize(); 
return EXIT_SUCCESS;
}

